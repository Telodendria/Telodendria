/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <Db.h>

#include <Memory.h>
#include <Json.h>
#include <Util.h>
#include <Str.h>

#include <sys/types.h>
#include <dirent.h>

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

struct Db
{
    char *dir;
    pthread_mutex_t lock;

    size_t cacheSize;
    size_t maxCache;
    HashMap *cache;

    /*
     * The cache uses a double linked list (see DbRef
     * below) to know which objects are most and least
     * recently used. The following diagram helps me
     * know what way all the pointers go, because it
     * can get very confusing sometimes. For example,
     * there's nothing stopping "next" from pointing to
     * least recent, and "prev" from pointing to most
     * recent, so hopefully this clarifies the pointer
     * terminology used when dealing with the linked
     * list:
     *
     *         mostRecent            leastRecent
     *             |   prev       prev   |   prev
     *           +---+ ---> +---+ ---> +---+ ---> NULL
     *           |ref|      |ref|      |ref|
     * NULL <--- +---+ <--- +---+ <--- +---+
     *      next       next       next
     */
    DbRef *mostRecent;
    DbRef *leastRecent;
};

struct DbRef
{
    HashMap *json;
    pthread_mutex_t lock;

    unsigned long ts;
    size_t size;

    Array *name;

    DbRef *prev;
    DbRef *next;

    FILE *fp;
};

static void
StringArrayFree(Array * arr)
{
    size_t i;

    for (i = 0; i < ArraySize(arr); i++)
    {
        Free(ArrayGet(arr, i));
    }

    ArrayFree(arr);
}

static ssize_t DbComputeSize(HashMap *);

static ssize_t
DbComputeSizeOfValue(JsonValue * val)
{
    MemoryInfo *a;
    ssize_t total = 0;

    size_t i;

    union
    {
        char *str;
        Array *arr;
    } u;

    if (!val)
    {
        return -1;
    }

    a = MemoryInfoGet(val);
    if (a)
    {
        total += MemoryInfoGetSize(a);
    }

    switch (JsonValueType(val))
    {
        case JSON_OBJECT:
            total += DbComputeSize(JsonValueAsObject(val));
            break;
        case JSON_ARRAY:
            u.arr = JsonValueAsArray(val);
            a = MemoryInfoGet(u.arr);

            if (a)
            {
                total += MemoryInfoGetSize(a);
            }

            for (i = 0; i < ArraySize(u.arr); i++)
            {
                total += DbComputeSizeOfValue(ArrayGet(u.arr, i));
            }
            break;
        case JSON_STRING:
            u.str = JsonValueAsString(val);
            a = MemoryInfoGet(u.str);
            if (a)
            {
                total += MemoryInfoGetSize(a);
            }
            break;
        case JSON_NULL:
        case JSON_INTEGER:
        case JSON_FLOAT:
        case JSON_BOOLEAN:
        default:
            /* These don't use any extra heap space */
            break;
    }
    return total;
}

static ssize_t
DbComputeSize(HashMap * json)
{
    char *key;
    JsonValue *val;
    MemoryInfo *a;
    size_t total;

    if (!json)
    {
        return -1;
    }

    total = 0;

    a = MemoryInfoGet(json);
    if (a)
    {
        total += MemoryInfoGetSize(a);
    }

    while (HashMapIterate(json, &key, (void **) &val))
    {
        a = MemoryInfoGet(key);
        if (a)
        {
            total += MemoryInfoGetSize(a);
        }

        total += DbComputeSizeOfValue(val);
    }

    return total;
}

static char *
DbHashKey(Array * args)
{
    size_t i;
    char *str = NULL;

    for (i = 0; i < ArraySize(args); i++)
    {
        char *tmp = StrConcat(2, str, ArrayGet(args, i));

        Free(str);
        str = tmp;
    }

    return str;
}

static char *
DbDirName(Db * db, Array * args, size_t strip)
{
    size_t i;
    char *str = StrConcat(2, db->dir, "/");

    for (i = 0; i < ArraySize(args) - strip; i++)
    {
        char *tmp;

        tmp = StrConcat(3, str, ArrayGet(args, i), "/");

        Free(str);

        str = tmp;
    }

    return str;
}

static char *
DbFileName(Db * db, Array * args)
{
    size_t i;
    char *str = StrConcat(2, db->dir, "/");

    for (i = 0; i < ArraySize(args); i++)
    {
        char *tmp;
        char *arg = StrDuplicate(ArrayGet(args, i));
        size_t j = 0;

        /* Sanitize name to prevent directory traversal attacks */
        while (arg[j])
        {
            switch (arg[j])
            {
                case '/':
                    arg[j] = '_';
                    break;
                case '.':
                    arg[j] = '-';
                    break;
                default:
                    break;
            }
            j++;
        }

        tmp = StrConcat(3, str, arg,
                        (i < ArraySize(args) - 1) ? "/" : ".json");

        Free(arg);
        Free(str);

        str = tmp;
    }

    return str;
}

static void
DbCacheEvict(Db * db)
{
    DbRef *ref = db->leastRecent;
    DbRef *tmp;

    while (ref && db->cacheSize > db->maxCache)
    {
        char *hash = DbHashKey(ref->name);

        if (pthread_mutex_trylock(&ref->lock) != 0)
        {
            /* This ref is locked by another thread, don't evict it. */
            ref = ref->next;
            continue;
        }

        JsonFree(ref->json);
        pthread_mutex_unlock(&ref->lock);
        pthread_mutex_destroy(&ref->lock);

        hash = DbHashKey(ref->name);
        HashMapDelete(db->cache, hash);
        Free(hash);

        StringArrayFree(ref->name);

        db->cacheSize -= ref->size;

        ref->next->prev = ref->prev;
        if (!ref->prev)
        {
            db->leastRecent = ref->next;
        }
        else
        {
            ref->prev->next = ref->next;
        }

        tmp = ref->next;
        Free(ref);

        ref = tmp;
    }
}

Db *
DbOpen(char *dir, size_t cache)
{
    Db *db;

    if (!dir)
    {
        return NULL;
    }

    db = Malloc(sizeof(Db));
    if (!db)
    {
        return NULL;
    }

    db->dir = dir;
    db->maxCache = cache;

    pthread_mutex_init(&db->lock, NULL);

    if (db->maxCache)
    {
        db->cache = HashMapCreate();
        if (!db->cache)
        {
            return NULL;
        }

        db->mostRecent = NULL;
        db->leastRecent = NULL;
    }
    else
    {
        db->cache = NULL;
    }

    return db;
}

void
DbClose(Db * db)
{
    char *key;
    DbRef *val;

    if (!db)
    {
        return;
    }

    pthread_mutex_destroy(&db->lock);

    while (HashMapIterate(db->cache, &key, (void **) &val))
    {
        JsonFree(val->json);
        StringArrayFree(val->name);
        pthread_mutex_destroy(&val->lock);
        Free(val);
    }
    HashMapFree(db->cache);

    Free(db);
}

static DbRef *
DbLockFromArr(Db * db, Array * args)
{
    char *file;
    char *hash;
    DbRef *ref;
    FILE *fp;
    struct flock lock;

    if (!db || !args)
    {
        return NULL;
    }

    ref = NULL;
    hash = NULL;

    pthread_mutex_lock(&db->lock);

    /* Check if the item is in the cache */
    hash = DbHashKey(args);
    ref = HashMapGet(db->cache, hash);
    file = DbFileName(db, args);

    /* Open the file for reading and writing so we can lock it */
    fp = fopen(file, "r+");
    if (!fp)
    {
        if (ref)
        {
            pthread_mutex_lock(&ref->lock);

            HashMapDelete(db->cache, hash);
            JsonFree(ref->json);
            StringArrayFree(ref->name);

            db->cacheSize -= ref->size;

            if (ref->next)
            {
                ref->next->prev = ref->prev;
            }
            else
            {
                db->mostRecent = ref->prev;
            }

            if (ref->prev)
            {
                ref->prev->next = ref->next;
            }
            else
            {
                db->leastRecent = ref->next;
            }

            pthread_mutex_unlock(&ref->lock);
            pthread_mutex_destroy(&ref->lock);
            Free(ref);
        }
        ref = NULL;
        goto finish;
    }

    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;

    /* Lock the file on the disk */
    if (fcntl(fileno(fp), F_SETLK, &lock) < 0)
    {
        fclose(fp);
        ref = NULL;
        goto finish;
    }

    if (ref)                       /* In cache */
    {
        unsigned long diskTs = UtilLastModified(file);

        pthread_mutex_lock(&ref->lock);
        ref->fp = fp;

        if (diskTs > ref->ts)
        {
            /* File was modified on disk since it was cached */
            HashMap *json = JsonDecode(fp);

            if (!json)
            {
                pthread_mutex_unlock(&ref->lock);
                fclose(fp);
                ref = NULL;
                goto finish;
            }

            JsonFree(ref->json);
            ref->json = json;
            ref->ts = diskTs;
            ref->size = DbComputeSize(ref->json);

        }

        /* Float this ref to mostRecent */
        if (ref->next)
        {
            ref->next->prev = ref->prev;

            if (!ref->prev)
            {
                db->leastRecent = ref->next;
            }
            else
            {
                ref->prev->next = ref->next;
            }

            ref->prev = db->mostRecent;
            ref->next = NULL;
            db->mostRecent = ref;
        }

        /* The file on disk may be larger than what we have in memory,
         * which may require items in cache to be evicted. */
        DbCacheEvict(db);
    }
    else
    {
        Array *name = ArrayCreate();
        size_t i;

        /* Not in cache; load from disk */

        ref = Malloc(sizeof(DbRef));
        if (!ref)
        {
            fclose(fp);
            goto finish;
        }

        ref->json = JsonDecode(fp);
        ref->fp = fp;

        if (!ref->json)
        {
            Free(ref);
            fclose(fp);
            ref = NULL;
            goto finish;
        }

        pthread_mutex_init(&ref->lock, NULL);
        pthread_mutex_lock(&ref->lock);


        for (i = 0; i < ArraySize(args); i++)
        {
            ArrayAdd(name, StrDuplicate(ArrayGet(args, i)));
        }
        ref->name = name;

        if (db->cache)
        {
            ref->ts = UtilServerTs();
            ref->size = DbComputeSize(ref->json);
            HashMapSet(db->cache, hash, ref);
            db->cacheSize += ref->size;

            ref->next = NULL;
            ref->prev = db->mostRecent;
            db->mostRecent = ref;

            /* Adding this item to the cache may case it to grow too
             * large, requiring some items to be evicted */
            DbCacheEvict(db);
        }
    }

finish:
    pthread_mutex_unlock(&db->lock);

    Free(file);
    Free(hash);

    return ref;
}

DbRef *
DbCreate(Db * db, size_t nArgs,...)
{
    FILE *fp;
    char *file;
    char *dir;
    va_list ap;
    Array *args;
    DbRef *ret;

    if (!db)
    {
        return NULL;
    }

    va_start(ap, nArgs);
    args = ArrayFromVarArgs(nArgs, ap);
    va_end(ap);

    if (!args)
    {
        return NULL;
    }

    file = DbFileName(db, args);

    if (UtilLastModified(file))
    {
        Free(file);
        ArrayFree(args);
        return NULL;
    }

    dir = DbDirName(db, args, 1);
    if (UtilMkdir(dir, 0750) < 0)
    {
        Free(file);
        ArrayFree(args);
        Free(dir);
        return NULL;
    }

    Free(dir);

    fp = fopen(file, "w");
    Free(file);
    if (!fp)
    {
        ArrayFree(args);
        return NULL;
    }

    fprintf(fp, "{}");
    fflush(fp);
    fclose(fp);

    ret = DbLockFromArr(db, args);

    ArrayFree(args);

    return ret;
}

int
DbDelete(Db * db, size_t nArgs,...)
{
    va_list ap;
    Array *args;
    char *file;
    char *hash;
    int ret = 1;
    DbRef *ref;

    if (!db)
    {
        return 0;
    }

    va_start(ap, nArgs);
    args = ArrayFromVarArgs(nArgs, ap);
    va_end(ap);

    pthread_mutex_lock(&db->lock);

    hash = DbHashKey(args);
    file = DbFileName(db, args);

    ref = HashMapGet(db->cache, hash);
    if (ref)
    {
        pthread_mutex_lock(&ref->lock);

        HashMapDelete(db->cache, hash);
        JsonFree(ref->json);
        StringArrayFree(ref->name);

        db->cacheSize -= ref->size;

        if (ref->next)
        {
            ref->next->prev = ref->prev;
        }
        else
        {
            db->mostRecent = ref->prev;
        }

        if (ref->prev)
        {
            ref->prev->next = ref->next;
        }
        else
        {
            db->leastRecent = ref->next;
        }

        pthread_mutex_unlock(&ref->lock);
        pthread_mutex_destroy(&ref->lock);
        Free(ref);
    }

    Free(hash);

    if (UtilLastModified(file))
    {
        ret = remove(file) == 0;
    }

    pthread_mutex_unlock(&db->lock);

    ArrayFree(args);
    Free(file);
    return ret;
}

DbRef *
DbLock(Db * db, size_t nArgs,...)
{
    va_list ap;
    Array *args;
    DbRef *ret;

    va_start(ap, nArgs);
    args = ArrayFromVarArgs(nArgs, ap);
    va_end(ap);

    if (!args)
    {
        return NULL;
    }

    ret = DbLockFromArr(db, args);

    ArrayFree(args);

    return ret;
}

int
DbUnlock(Db * db, DbRef * ref)
{
    if (!db || !ref)
    {
        return 0;
    }

    pthread_mutex_lock(&db->lock);

    rewind(ref->fp);
    if (ftruncate(fileno(ref->fp), 0) < 0)
    {
        pthread_mutex_unlock(&db->lock);
        return 0;
    }

    JsonEncode(ref->json, ref->fp, JSON_DEFAULT);

    fflush(ref->fp);
    fclose(ref->fp);

    if (db->cache)
    {
        db->cacheSize -= ref->size;
        ref->size = DbComputeSize(ref->json);
        db->cacheSize += ref->size;

        /* If this ref has grown significantly since we last computed
         * its size, it may have filled the cache and require some
         * items to be evicted. */
        DbCacheEvict(db);
        pthread_mutex_unlock(&ref->lock);
    }
    else
    {
        JsonFree(ref->json);
        StringArrayFree(ref->name);

        pthread_mutex_unlock(&ref->lock);
        pthread_mutex_destroy(&ref->lock);

        Free(ref);
    }

    pthread_mutex_unlock(&db->lock);
    return 1;
}

int
DbExists(Db * db, size_t nArgs,...)
{
    va_list ap;
    Array *args;
    char *file;
    int ret;

    va_start(ap, nArgs);
    args = ArrayFromVarArgs(nArgs, ap);
    va_end(ap);

    if (!args)
    {
        return 0;
    }

    /*
     * Though it's not explicitly required, we lock the
     * database before checking that an object exists to
     * prevent any potential race conditions.
     */
    pthread_mutex_lock(&db->lock);

    file = DbFileName(db, args);
    ret = UtilLastModified(file);

    pthread_mutex_unlock(&db->lock);

    Free(file);
    ArrayFree(args);

    return ret;
}

Array *
DbList(Db * db, size_t nArgs,...)
{
    Array *result;
    Array *path;
    DIR *files;
    struct dirent *file;
    char *dir;
    va_list ap;

    if (!db || !nArgs)
    {
        return NULL;
    }

    result = ArrayCreate();
    if (!result)
    {
        return NULL;
    }

    va_start(ap, nArgs);
    path = ArrayFromVarArgs(nArgs, ap);
    dir = DbDirName(db, path, 0);

    files = opendir(dir);
    if (!files)
    {
        ArrayFree(path);
        ArrayFree(result);
        Free(dir);
        return NULL;
    }
    while ((file = readdir(files)))
    {
        size_t namlen = strlen(file->d_name);

        if (namlen > 5)
        {
            int nameOffset = namlen - 5;

            if (strcmp(file->d_name + nameOffset, ".json") == 0)
            {
                file->d_name[nameOffset] = '\0';
                ArrayAdd(result, StrDuplicate(file->d_name));
            }
        }
    }
    closedir(files);

    ArrayFree(path);
    Free(dir);

    return result;
}

void
DbListFree(Array * arr)
{
    StringArrayFree(arr);
}

HashMap *
DbJson(DbRef * ref)
{
    return ref ? ref->json : NULL;
}
