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

#ifndef CYTOPLASM_HASHMAP_H
#define CYTOPLASM_HASHMAP_H

/***
 * @Nm HashMap
 * @Nd A simple hash map implementation.
 * @Dd October 11 2022
 * @Xr Array Queue
 *
 * This is the public interface for Cytoplasm's hash map
 * implementation. This hash map is designed to be simple,
 * well-documented, and generally readable and understandable, yet also
 * performant enough to be useful, because it is used extensively
 * throughout the project.
 * .Pp
 * Fundamentally, this is an entirely generic map implementation. It
 * can be used for many general purposes, but it is designed only to
 * implement the features Cytoplasm needs to be functional. One
 * example of a Cytoplasm-specific feature is that keys cannot be
 * arbitrary data; they are NULL-terminated C strings.
 */

#include <stddef.h>

/**
 * These functions operate on an opaque structure, which the caller
 * has no knowledge about.
 */
typedef struct HashMap HashMap;

/**
 * Create a new hash map that is ready to be used with the rest of the
 * functions defined here.
 */
extern HashMap * HashMapCreate(void);

/**
 * Free the specified hash map such that it becomes invalid and any
 * future use results in undefined behavior. Note that this function
 * does not free the values stored in the hash map, but since it stores
 * the keys internally, it will free the keys. You should use
 * .Fn HashMapIterate
 * to free the values stored in this map appropriately before calling
 * this function.
 */
extern void HashMapFree(HashMap *);

/**
 * Control the maximum load of the hash map before it is expanded.
 * When the hash map reaches the given capacity, it is grown. You
 * don't want to only grow hash maps when they are full, because that
 * makes them perform very poorly. The maximum load value is a
 * percentage of how full the hash map is, and it should be between
 * 0 and 1, where 0 means that no elements will cause the map to be
 * expanded, and 1 means that the hash map must  be completely full
 * before it is expanded. The default maximum load on a new hash map
 * is 0.75, which should be good enough for most purposes, however,
 * this function exists specifically so that the maximum load can be
 * fine-tuned.
 */
extern void HashMapMaxLoadSet(HashMap *, float);

/**
 * Use a custom hashing function with the given hash map. New hash
 * maps have a sane hashing function that should work okay for most
 * use cases, but if you have a better hashing function, it can be
 * specified this way. Do not change the hash function after keys have
 * been added; doing so results in undefined behavior. Only set a new
 * hash function immediately after constructing a new hash map, before
 * anything has been added to it.
 * .Pp
 * The hash function takes a pointer to a C string, and is expected
 * to return a fairly unique numerical hash value which will be
 * converted into an array index.
 */
extern void
HashMapFunctionSet(HashMap *, unsigned long (*) (const char *));

/**
 * Set the given string key to the given value. Note that the key is
 * copied into the hash map's own memory space, but the value is not.
 * It is the caller's job to ensure that the value pointer remains
 * valid for the life of the hash map, and are freed when no longer
 * needed.
 */
extern void * HashMapSet(HashMap *, char *, void *);

/**
 * Retrieve the value for the given key, or return NULL if no such
 * key exists in the hash map.
 */
extern void * HashMapGet(HashMap *, const char *);

/**
 * Remove a value specified by the given key from the hash map, and
 * return it to the caller to deal with. This function returns NULL
 * if no such key exists.
 */
extern void * HashMapDelete(HashMap *, const char *);

/**
 * Iterate over all the keys and values of a hash map. This function
 * works very similarly to
 * .Xr getopt 3 ,
 * where calls are repeatedly made in a while loop until there are no
 * more items to go over. The difference is that this function does not
 * rely on globals; it takes pointer pointers, and stores all
 * necessary state inside the hash map itself.
 * .Pp
 * Note that this function is not thread-safe; two threads cannot be
 * iterating over any given hash map at the same time, though they
 * can each be iterating over different hash maps.
 * .Pp
 * This function can be tricky to use in some scenarios, as it
 * continues where it left off on each call, until there are no more
 * elements to go through in the hash map. If you are not iterating
 * over the entire map in one go, and happen to break the loop, then
 * the next time you attempt to iterate the hash map, you'll start
 * somewhere in the middle, which is most likely not the intended
 * behavior. Thus, it is always recommended to iterate over the entire
 * hash map if you're going to use this function.
 * .Pp
 * Also note that the behavior of this function is undefined if
 * insertions or deletions occur during the iteration. This
 * functionality has not been tested, and will likely not work.
 */
extern int HashMapIterate(HashMap *, char **, void **);

/**
 * A reentrant version of
 * .Fn HashMapIterate
 * that allows the caller to overcome the flaws of that function by
 * storing the cursor outside of the hash map structure itself. This
 * allows multiple threads to iterate over the same hash map at the
 * same time, and it allows the iteration to be halted midway through
 * without causing any unintended side effects.
 * .Pp
 * The cursor should be initialized to 0 at the start of iteration.
 */
extern int
HashMapIterateReentrant(HashMap *, char **, void **, size_t *);

#endif                             /* CYTOPLASM_HASHMAP_H */
