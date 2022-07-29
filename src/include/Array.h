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
 * Array.h: A simple array data structure that is automatically
 * resized when new values are added. This implementation does not
 * actually store the values of the items it; it only stores pointers
 * to the data. As such, you will still have to manually maintain all
 * your data. The advantage of this is that this array implementation
 * doesn't have to copy items, and thus doesn't have to know how big
 * they are.
 *
 * This array is optimized for storage space and appending. Deletions
 * are expensive in that all the items of the list are moved down
 * to fill the hole where the deletion occurred. Insertions are
 * also expensive in that all the elements are shifted to make room
 * for the new element.
 *
 * Due to these, this array implementation is really intended to be
 * used primarily for linear writing and linear or random reading.
 */
#ifndef TELODENDRIA_ARRAY_H
#define TELODENDRIA_ARRAY_H

#include <stddef.h>

typedef struct Array Array;

/*
 * Create a new, empty array on the heap.
 *
 * Params: none
 *
 * Return: A pointer to an Array, or NULL if there was an error
 * allocating memory for the Array.
 */
extern Array *
 ArrayCreate(void);

/*
 * Get the size of the provided array. Note that this is the number
 * of elements in the array, not how much memory has been allocated
 * for it.
 *
 * This is an extremely cheap operation, because the size does not
 * need to be computed; rather it is just pulled right out of the
 * Array and returned to the caller.
 *
 * Params:
 *
 *   (Array *) The array to check the size of.
 *
 * Return: The number of elements in the provided array, or 0 if the
 * provided array is NULL.
 */
extern size_t
 ArraySize(Array *);

/*
 * Get an element out of the provided array at the provided index.
 *
 * Params:
 *
 *   (Array *) The array to get an element from.
 *   (size_t)  The index of the array where the desired element is
 *             located.
 *
 * Return: A pointer to the data located at the given index, or NULL
 * if no array was provided, or the index is greater than or equal
 * to the size of the array.
 */
extern void *
 ArrayGet(Array *, size_t);

/*
 * Insert an element into the array at the given index.
 *
 * Params:
 *
 *   (Array *) The array to get the element from.
 *   (void *)  The value to insert into the array.
 *   (size_t)  The index at which the given value.
 *
 * Return: A boolean value that indicates whether or not the insert
 * was successful. A return value of 0 indicates that the insert was
 * NOT successful, and a return value of anything else indicates that
 * the insert was successful.
 */
extern int
 ArrayInsert(Array *, void *, size_t);

/*
 * Append an element to the end of the array. This function actually
 * uses ArrayInsert() under the hood, but it makes appending to an
 * array more convenient because you don't necessarily have to keep
 * track of the array's size.
 *
 * Params:
 *
 *   (Array *) The array to append to.
 *   (void *)  The value to append to the array.
 *
 * Return: The result of appending the element, which is the same as
 * a call to ArrayInsert().
 */
extern int
 ArrayAdd(Array *, void *);

/*
 * Delete an element from an array by shifting all the elements that
 * come after it down one index.
 *
 * Params:
 *
 *   (Array *) The array to delete a value from.
 *   (size_t)  The desired index to delete. All elements above this
 *             index are then shifted down to fill the gap.
 *
 * Return: A pointer to the deleted element, so that it can be freed
 * or otherwise dealt with, or NULL if the array is NULL or the index
 * is out of bounds.
 */
extern void *
 ArrayDelete(Array *, size_t);

/*
 * Sort the array using a simple quick-sort algorithm. This function
 * works by taking a caller-specified compare function, so that no
 * assumptions about the data stored in the array need to be made by
 * this code.
 *
 * Params:
 *
 *   (Array *) The array to sort. Note that the sort will be done
 *             in-place.
 *   (int (*)(void *, void *)) A function that takes in two void
 *             pointers and returns an integer. This function is
 *             responsible for comparing the passed items, and
 *             returning a code that indicates how they should be
 *             ordered. A return value of 0 indicates that the two
 *             items are identical. A return value greater than 0
 *             indicates that the first item is "bigger" than the
 *             second item and should thus appear after it in the
 *             array, and a return value less than zero indicates
 *             the opposite: that the second element should appear
 *             after the first in the array.
 *
 */
extern void
 ArraySort(Array *, int (*) (void *, void *));

/*
 * Free all the memory associated with the given array. Note that this
 * does not free any of the values themselves; you should explicitly
 * iterate over the array and free all the values stored inside it
 * before calling this function, otherwise you may lose all the
 * pointers the array contains, and thus have a memory leak.
 *
 * Params:
 *
 *   (Array *) The array to free.
 *
 */
extern void
 ArrayFree(Array *);

/*
 * "Trim" the array by reallocating it with only the memory it needs
 * to hold the items it currently has. This function might be beneficial
 * to call on long-lived arrays that will be read-only, because it
 * frees any memory on the end of the array that isn't being used. The
 * array resizing algorithm will most likely allocate too much memory
 * for most arrays as elements are added, because there's no way to
 * know exactly how many elements will be stored.
 *
 * For example, a library that generates an Array to return to the
 * user may wish to call this function on it right before returning to
 * the caller if it is not expected that the caller will be modifying
 * the array and may hang on to it for a long time.
 *
 * Params:
 *
 *   (Array *) The array to trim extra memory (if any) off the end.
 *             Note that the array will still be fully-functional; if
 *             you add more elements, then more memory will be
 *             allocated like normal.
 *
 * Return: Whether or not the trim was successful. The trim may fail if
 * realloc() fails, or NULL was passed for the array. If realloc()
 * fails, this function is careful not to clobber the array. Upon
 * failure of this function, the array is guaranteed to be unaltered.
 */
extern int
 ArrayTrim(Array *);

#endif
