#include <HashMap.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct HashMapBucket
{
    uint32_t hash;
    char *key;
    void *value;
} HashMapBucket;

struct HashMap
{
    size_t count;
    size_t capacity;
    HashMapBucket **entries;

    float maxLoad;
    size_t iterator;
};

static uint32_t
HashMapHashKey(const char *key)
{
    uint32_t hash = 2166136261u;
    size_t i = 0;

    while (key[i])
    {
        hash ^= (uint8_t) key[i];
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

    newEntries = calloc(map->capacity, sizeof(HashMapBucket *));
    if (!newEntries)
    {
        return 0;
    }

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
                        free(newEntries[index]);
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
            free(map->entries[i]);
        }
    }

    free(map->entries);
    map->entries = newEntries;
    return 1;
}

HashMap *
HashMapCreate(void)
{
    HashMap *map = malloc(sizeof(HashMap));

    if (!map)
    {
        return NULL;
    }

    map->maxLoad = 0.75;
    map->count = 0;
    map->capacity = 16;
    map->iterator = 0;

    map->entries = calloc(map->capacity, sizeof(HashMapBucket *));
    if (!map->entries)
    {
        free(map);
        return NULL;
    }

    return map;
}

void *
HashMapDelete(HashMap * map, const char *key)
{
    uint32_t hash;
    size_t index;

    if (!map || !key)
    {
        return NULL;
    }

    hash = HashMapHashKey(key);
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
                free(map->entries[i]);
            }
        }
    }

    free(map);
}

void *
HashMapGet(HashMap * map, const char *key)
{
    uint32_t hash;
    size_t index;

    if (!map || !key)
    {
        return NULL;
    }

    hash = HashMapHashKey(key);
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
HashMapIterate(HashMap * map, char **key, void **value)
{
    if (!map)
    {
        return 0;
    }

    if (map->iterator >= map->capacity)
    {
        map->iterator = 0;
        *key = NULL;
        *value = NULL;
        return 0;
    }

    while (map->iterator < map->capacity)
    {
        HashMapBucket *bucket = map->entries[map->iterator];

        map->iterator++;

        if (bucket)
        {
            *key = bucket->key;
            *value = bucket->value;
            return 1;
        }
    }

    map->iterator = 0;
    return 0;
}

void
HashMapMaxLoadSet(HashMap * map, float load)
{
    if (!map)
    {
        return;
    }

    map->maxLoad = load;
}


void *
HashMapSet(HashMap * map, char *key, void *value)
{
    uint32_t hash;
    size_t index;

    if (!map || !key || !value)
    {
        return NULL;
    }

    if (map->count + 1 > map->capacity * map->maxLoad)
    {
        HashMapGrow(map);
    }

    hash = HashMapHashKey(key);
    index = hash % map->capacity;

    for (;;)
    {
        HashMapBucket *bucket = map->entries[index];

        if (!bucket)
        {
            bucket = malloc(sizeof(HashMapBucket));
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
            bucket->key = key;
            bucket->value = value;
            break;
        }

        if (bucket->hash == hash)
        {
            void *oldValue = bucket->value;

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
        free(key);
    }

    if (value)
    {
        free(value);
    }
}
