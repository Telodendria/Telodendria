/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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

#include <pthread.h>

struct Db
{
    char *dir;
    size_t cacheSize;
    size_t maxCache;
    pthread_mutex_t lock;

    HashMap *cache;
};

struct DbRef
{
    HashMap *json;
    pthread_mutex_t lock;

    unsigned long ts;
    size_t size;

    char *prefix;
    char *key;
};

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
DbHashKey(char *prefix, char *key)
{
    return UtilStringConcat(prefix, key);
}

static char *
DbFileName(Db * db, char *prefix, char *key)
{
    char *tmp = UtilStringConcat(prefix, "/");
    char *tmp2 = UtilStringConcat(tmp, key);
    char *tmp3 = UtilStringConcat(tmp2, ".json");

    char *tmp4 = UtilStringConcat(db->dir, "/");
    char *tmp5 = UtilStringConcat(tmp4, tmp3);

    Free(tmp);
    Free(tmp2);
    Free(tmp3);
    Free(tmp4);

    return tmp5;
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

    if (db->cache)
    {
        while (HashMapIterate(db->cache, &key, (void **) &val))
        {
            Free(key);
            JsonFree(val->json);
            Free(val->prefix);
            Free(val->key);
            pthread_mutex_destroy(&val->lock);
            Free(val);
        }
        HashMapFree(db->cache);
    }

    Free(db);
}

DbRef *
DbCreate(Db * db, char *prefix, char *key)
{
    FILE *fp;
    char *file;

    if (!db || !prefix || !key)
    {
        return NULL;
    }

    file = DbFileName(db, prefix, key);

    if (UtilLastModified(file) || UtilMkdir(prefix, 0750) < 0)
    {
        Free(file);
        return NULL;
    }

    fp = fopen(file, "w");
    Free(file);
    if (!fp)
    {
        return NULL;
    }

    fprintf(fp, "{}");
    fflush(fp);
    fclose(fp);

    return DbLock(db, prefix, key);
}

DbRef *
DbLock(Db * db, char *prefix, char *key)
{
    char *file;
    char *hash;
    DbRef *ref;

    if (!db || !prefix || !key)
    {
        return NULL;
    }

    ref = NULL;
    hash = NULL;

    pthread_mutex_lock(&db->lock);

    /* Check if the item is in the cache */
    if (db->cache)
    {
        hash = DbHashKey(prefix, key);
        ref = HashMapGet(db->cache, hash);
    }

    file = DbFileName(db, prefix, key);

    if (ref)                       /* In cache */
    {
        unsigned long diskTs = UtilLastModified(file);

        if (diskTs > ref->ts)
        {
            /* File was modified on disk since it was cached */
            FILE *fp = fopen(file, "r");
            HashMap *json;

            if (!fp)
            {
                ref = NULL;
                goto finish;
            }

            json = JsonDecode(fp);
            fclose(fp);

            if (!json)
            {
                ref = NULL;
                goto finish;
            }

            JsonFree(ref->json);
            ref->json = json;
            ref->ts = diskTs;
            ref->size = DbComputeSize(ref->json);
            ref->prefix = UtilStringDuplicate(prefix);
            ref->key = UtilStringDuplicate(key);
        }
    }
    else
    {
        /* Not in cache; load from disk */
        FILE *fp = fopen(file, "r");

        if (!fp)
        {
            ref = NULL;
            goto finish;
        }

        ref = Malloc(sizeof(DbRef));
        if (!ref)
        {
            fclose(fp);
            goto finish;
        }

        ref->json = JsonDecode(fp);
        fclose(fp);

        if (!ref->json)
        {
            Free(ref);
            ref = NULL;
            goto finish;
        }

        pthread_mutex_init(&ref->lock, NULL);
        ref->ts = UtilServerTs();
        ref->size = DbComputeSize(ref->json);
        ref->prefix = prefix;
        ref->key = key;

        /* If cache is enabled, cache this ref */
        if (db->cache)
        {
            HashMapSet(db->cache, hash, ref);
            db->cacheSize += ref->size;

            /* Cache is full; evict old items */
            if (db->cacheSize > db->maxCache)
            {
                /* TODO */
            }
        }
    }

    pthread_mutex_lock(&ref->lock);

finish:
    pthread_mutex_unlock(&db->lock);

    Free(file);
    Free(hash);
    return ref;
}

int
DbUnlock(Db * db, DbRef * ref)
{
    FILE *fp;
    char *file;

    if (!db || !ref)
    {
        return 0;
    }

    pthread_mutex_lock(&db->lock);

    file = DbFileName(db, ref->prefix, ref->key);
    fp = fopen(file, "w");
    if (!fp)
    {
        return 0;
    }

    JsonEncode(ref->json, fp);
    fflush(fp);
    fclose(fp);

    pthread_mutex_unlock(&ref->lock);

    /* No cache, free it now */
    if (!db->cache)
    {
        JsonFree(ref->json);
        Free(ref->prefix);
        Free(ref->key);
        pthread_mutex_destroy(&ref->lock);
        Free(ref);
    }

    return 1;
}

HashMap *
DbJson(DbRef * ref)
{
    return ref ? ref->json : NULL;
}
