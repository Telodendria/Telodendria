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
#include <HashMap.h>

#include <Memory.h>
#include <Str.h>

#include <stddef.h>
#include <string.h>

typedef struct HashMapBucket
{
    unsigned long hash;
    char *key;
    void *value;
} HashMapBucket;

struct HashMap
{
    size_t count;
    size_t capacity;
    HashMapBucket **entries;

    unsigned long (*hashFunc) (const char *);

    float maxLoad;
    size_t iterator;
};

static unsigned long
HashMapHashKey(const char *key)
{
    unsigned long hash = 2166136261u;
    size_t i = 0;

    while (key[i])
    {
        hash ^= (unsigned char) key[i];
        hash *= 16777619;

        i++;
    }

    return hash;
}

static int
HashMapGrow(HashMap * map)
{
    size_t oldCapacity;
    size_t i;
    HashMapBucket **newEntries;

    if (!map)
    {
        return 0;
    }

    oldCapacity = map->capacity;
    map->capacity *= 2;

    newEntries = Malloc(map->capacity * sizeof(HashMapBucket *));
    if (!newEntries)
    {
        map->capacity /= 2;
        return 0;
    }

    memset(newEntries, 0, map->capacity * sizeof(HashMapBucket *));

    for (i = 0; i < oldCapacity; i++)
    {
        /* If there is a value here, and it isn't a tombstone */
        if (map->entries[i] && map->entries[i]->hash)
        {
            /* Copy it to the new entries array */
            size_t index = map->entries[i]->hash % map->capacity;

            for (;;)
            {
                if (newEntries[index])
                {
                    if (!newEntries[index]->hash)
                    {
                        Free(newEntries[index]);
                        newEntries[index] = map->entries[i];
                        break;
                    }
                }
                else
                {
                    newEntries[index] = map->entries[i];
                    break;
                }

                index = (index + 1) % map->capacity;
            }
        }
        else
        {
            /* Either NULL or a tombstone */
            Free(map->entries[i]);
        }
    }

    Free(map->entries);
    map->entries = newEntries;
    return 1;
}

HashMap *
HashMapCreate(void)
{
    HashMap *map = Malloc(sizeof(HashMap));

    if (!map)
    {
        return NULL;
    }

    map->maxLoad = 0.75;
    map->count = 0;
    map->capacity = 16;
    map->iterator = 0;
    map->hashFunc = HashMapHashKey;

    map->entries = Malloc(map->capacity * sizeof(HashMapBucket *));
    if (!map->entries)
    {
        Free(map);
        return NULL;
    }

    memset(map->entries, 0, map->capacity * sizeof(HashMapBucket *));

    return map;
}

void *
HashMapDelete(HashMap * map, const char *key)
{
    unsigned long hash;
    size_t index;

    if (!map || !key)
    {
        return NULL;
    }

    hash = map->hashFunc(key);
    index = hash % map->capacity;

    for (;;)
    {
        HashMapBucket *bucket = map->entries[index];

        if (!bucket)
        {
            break;
        }

        if (bucket->hash == hash)
        {
            bucket->hash = 0;
            return bucket->value;
        }

        index = (index + 1) % map->capacity;
    }

    return NULL;
}

void
HashMapFree(HashMap * map)
{
    if (map)
    {
        size_t i;

        for (i = 0; i < map->capacity; i++)
        {
            if (map->entries[i])
            {
                Free(map->entries[i]->key);
                Free(map->entries[i]);
            }
        }
        Free(map->entries);
        Free(map);
    }
}

void *
HashMapGet(HashMap * map, const char *key)
{
    unsigned long hash;
    size_t index;

    if (!map || !key)
    {
        return NULL;
    }

    hash = map->hashFunc(key);
    index = hash % map->capacity;

    for (;;)
    {
        HashMapBucket *bucket = map->entries[index];

        if (!bucket)
        {
            break;
        }

        if (bucket->hash == hash)
        {
            return bucket->value;
        }

        index = (index + 1) % map->capacity;
    }

    return NULL;
}

int
HashMapIterateReentrant(HashMap * map, char **key, void **value, size_t * i)
{
    if (!map)
    {
        return 0;
    }

    if (*i >= map->capacity)
    {
        *i = 0;
        *key = NULL;
        *value = NULL;
        return 0;
    }

    while (*i < map->capacity)
    {
        HashMapBucket *bucket = map->entries[*i];

        *i = *i + 1;

        if (bucket && bucket->hash)
        {
            *key = bucket->key;
            *value = bucket->value;
            return 1;
        }
    }

    *i = 0;
    return 0;
}

int
HashMapIterate(HashMap * map, char **key, void **value)
{
    if (!map)
    {
        return 0;
    }
    else
    {
        return HashMapIterateReentrant(map, key, value, &map->iterator);
    }
}

void
HashMapMaxLoadSet(HashMap * map, float load)
{
    if (!map || (load > 1.0 || load <= 0))
    {
        return;
    }

    map->maxLoad = load;
}

void
HashMapFunctionSet(HashMap * map, unsigned long (*hashFunc) (const char *))
{
    if (!map || !hashFunc)
    {
        return;
    }

    map->hashFunc = hashFunc;
}

void *
HashMapSet(HashMap * map, char *key, void *value)
{
    unsigned long hash;
    size_t index;

    if (!map || !key || !value)
    {
        return NULL;
    }

    key = StrDuplicate(key);
    if (!key)
    {
        return NULL;
    }

    if (map->count + 1 > map->capacity * map->maxLoad)
    {
        HashMapGrow(map);
    }

    hash = map->hashFunc(key);
    index = hash % map->capacity;

    for (;;)
    {
        HashMapBucket *bucket = map->entries[index];

        if (!bucket)
        {
            bucket = Malloc(sizeof(HashMapBucket));
            if (!bucket)
            {
                break;
            }

            bucket->hash = hash;
            bucket->key = key;
            bucket->value = value;
            map->entries[index] = bucket;
            map->count++;
            break;
        }

        if (!bucket->hash)
        {
            bucket->hash = hash;
            Free(bucket->key);
            bucket->key = key;
            bucket->value = value;
            break;
        }

        if (bucket->hash == hash)
        {
            void *oldValue = bucket->value;

            Free(bucket->key);
            bucket->key = key;

            bucket->value = value;
            return oldValue;
        }

        index = (index + 1) % map->capacity;
    }

    return NULL;
}

void
HashMapIterateFree(char *key, void *value)
{
    if (key)
    {
        Free(key);
    }

    if (value)
    {
        Free(value);
    }
}
