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
 * Create a new HashMap object.
 *
 * Return: A HashMap object that is ready to be used by the rest of
 * the HashMap functions, or NULL if memory could not be allocated on
 * the heap.
 */
extern HashMap *
 HashMapCreate(void);

/*
 * Set the maximum load of the hash map before it is expanded. When the
 * hash map reaches the given capacity, it is grown. You don't want
 * to only grow hash maps when their full, because that makes them
 * perform very poorly.
 *
 * The default max load on new HashMap objects is 0.75, which should be
 * good enough for most purposes, but if you need finer tuning, feel
 * free to play with the max load with this function. The changes take
 * effect on the next insert.
 *
 * Params:
 *
 *   (HashMap *) The HashMap to modify the maximum load for.
 *   (float)          The new maximum load. This should be a value
 *                    between 0 and 1, which specifies the percentange
 *                    of "fullness" at which the HashMap will be
 *                    expanded. If the new max load is out of bounds,
 *                    this function does nothing.
 */
extern void
 HashMapMaxLoadSet(HashMap *, float);

/*
 * Set the given key in the HashMap to the given value. Note that the
 * key nor the value is copied into the HashMap's own memory space;
 * only pointers is stored. It is the caller's job to ensure that the
 * key and value memory remains valid for the life of the HashMap, and
 * are freed when they're no longer needed.
 *
 * Params:
 *
 *   (HashMap *) The hash map to set a key in.
 *   (char *)    The key to set.
 *   (void *)    The value to set at the given key.
 *
 * Return: The previous value at the given key, or NULL if the key did
 * not previously exist or any of the parameters provided are NULL. All
 * keys must have values; you can't set a key to NULL. To delete a key,
 * use HashMapDelete().
 */
extern void *
 HashMapSet(HashMap *, char *, void *);

/*
 * Get the value for the given key.
 *
 * Params:
 *
 *   (HashMap *) The hash map to check.
 *   (char *)    The key to get the value for.
 *
 * Return: The value at the given key, or NULL if the key does not
 * exist, no map was provided, or no key was provided.
 */
extern void *
 HashMapGet(HashMap *, const char *);

/*
 * Delete the value for the given key.
 *
 * Params:
 *
 *   (HashMap *) The map to delete the given key from.
 *   (const char *) The key to delete.
 *
 * Return: The value at the given key, or NULL if the key does not
 * exist or the map or key was not provided.
 */
extern void *
 HashMapDelete(HashMap *, const char *);

/*
 * Iterate over all the keys and values of a hash map. This function
 * works similarly to the POSIX getopt(), where calls are repeatedly
 * made in a "while" loop until there are no more items to go over.
 *
 * The difference is that this function does not rely on globals. This
 * function takes pointer pointers, and stores necessary state inside
 * the hash map structure itself.
 *
 * This function can be tricky to use in some scenarios, as it
 * continues where it left off on each call, until there are no more
 * elements. Then it returns 0 and resets the iterator, so that it can
 * start over for the next iteration. This means that if you are not
 * iterating over the entire map at one, and break the loop, the next
 * time you try to iterate the HashMap, you'll start somewhere in the
 * middle. Thus, it's recommended to iterate over the entire map. For
 * scenarios in which the entire map needs to be iterated, such as
 * when freeing all the keys and values, this function does well.
 *
 * Params:
 *
 *   (HashMap *) The hash map to iterate over.
 *   (char **)   A character pointer that will be set to the current
 *               key.
 *   (void **)   A void pointer that will be set to the current value.
 *
 * Return: 1 if there are still elements left in this iteration of the
 * hash map, or 0 if no valid hash map was provided, or there are no
 * more elements in it for this iteration. Note that after this
 * function returns 0 on a hash map, subsequent iterations will start
 * from the beginning.
 */
extern int
 HashMapIterate(HashMap *, char **, void **);

/*
 * Free the hash map, returning its memory to the operating system.
 * Note that this function does not free the keys or values stored in
 * the map since this hash map implementation has no way of knowing
 * what actually is stored in it. You should use HashMapIterate() to
 * free the values using your own algorithm.
 *
 * Params:
 *
 *   (HashMap *) The hash map to free. The pointer can be safely
 *               discarded when this function returns. In fact,
 *               accessing it after this function returns is undefined
 *               behavior.
 */
extern void
 HashMapFree(HashMap *);

#endif                             /* TELODENDRIA_HASHMAP_H */
