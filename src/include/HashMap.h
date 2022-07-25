/*
 * HashMap.h: The public interface for Telodendria's hash map
 * implementation. This hash map is designed to be simple, well
 * documented, and generally readable and understandable, yet also
 * performant enough to be useful, because it is used extensively in
 * Telodendria.
 *
 * Fundamentally, this is an entirely generic map implementation. It
 * can be used for many general purposes, but it is designed to only
 * implement the features that Telodendria needs to function well.
 *
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
 */
#ifndef TELODENDRIA_HASHMAP_H
#define TELODENDRIA_HASHMAP_H

#include <stddef.h>

/*
 * HashMap: The primary data structure used by this hash map algorithm.
 * All data necessary for the algorithm to function is stored inside
 * it. This is an opaque structure; use the methods defined by this
 * interface to manipulate it.
 */
typedef struct HashMap HashMap;

/*
 * HashMapCreate: Create a new HashMap object.
 *
 * Returns: A HashMap object that is ready to be used by the rest of
 * the HashMap functions, or NULL if memory could not be allocated on
 * the heap.
 */
extern HashMap *
 HashMapCreate(void);

extern void
 HashMapMaxLoadSet(HashMap * map, float load);

/*
 * HashMapSet: Set the given key in the HashMap to the given value. Note
 * that the value is not copied into the HashMap's own memory space;
 * only the pointer is stored. It is the caller's job to ensure that the
 * value's memory remains valid for the life of the HashMap.
 *
 * Returns: The previous value at the given key, or NULL if the key did
 * not previously exist or any of the parameters provided are NULL. All
 * keys must have values; you can't set a key to NULL. To delete a key,
 * use HashMapDelete.
 */
extern void *
 HashMapSet(HashMap * map, char *key, void *value);

/*
 * HashMapGet: Get the value for the given key.
 *
 * Returns: The value at the given key, or NULL if the key does not
 * exist, no map was provided, or no key was provided.
 */
extern void *
 HashMapGet(HashMap * map, const char *key);

/*
 * HashMapDelete: Delete the value for the given key.
 *
 * Returns: The value at the given key, or NULL if the key does not
 * exist or the map or key was not provided.
 */
extern void *
 HashMapDelete(HashMap * map, const char *key);

extern void
 HashMapIterate(HashMap * map, void (*iteratorFunc) (char *, void *));

/*
 * HashMapFree: Free the hash map, returning its memory to the operating
 * system. Note that this function does not free the values stored in
 * the map since this hash map implementation has no way of knowing
 * what actually is stored in it. You should use HashMapIterate to
 * free the values using your own algorithm.
 */
extern void
 HashMapFree(HashMap * map);

#endif                             /* TELODENDRIA_HASHMAP_H */
