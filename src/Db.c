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

#include <pthread.h>

struct Db
{
    const char *dir;
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
    char *file;
};

Db *
DbOpen(const char *dir, size_t cache)
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

    return db;
}

void
DbClose(Db * db)
{
    if (!db)
    {
        return;
    }

    pthread_mutex_destroy(&db->lock);
    HashMapFree(db->cache);

    Free(db);
}

DbRef *
DbCreate(Db * db, char *prefix, char *key)
{
    if (!db || !prefix || !key)
    {
        return NULL;
    }

    return NULL;
}

DbRef *
DbLock(Db * db, char *prefix, char *key)
{
    if (!db || !prefix || !key)
    {
        return NULL;
    }

    pthread_mutex_lock(&db->lock);

    pthread_mutex_unlock(&db->lock);
    return NULL;
}

void
DbUnlock(Db * db, DbRef * ref)
{
    if (!db || !ref)
    {
        return;
    }

    pthread_mutex_lock(&db->lock);

    pthread_mutex_unlock(&db->lock);

    return;
}

HashMap *
DbJson(DbRef * ref)
{
    return ref ? ref->json : NULL;
}
