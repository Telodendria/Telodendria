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

#ifndef CYTOPLASM_ARRAY_H
#define CYTOPLASM_ARRAY_H

/***
 * @Nm Array
 * @Nd A simple dynamic array data structure.
 * @Dd November 24 2022
 * @Xr HashMap Queue
 *
 * These functions implement a simple array data structure that is
 * automatically resized as necessary when new values are added.  This
 * implementation does not actually store the values of the items in it;
 * it only stores pointers to the data. As such, you will still have to
 * manually maintain all your data. The advantage of this is that these
 * functions don't have to copy data, and thus don't care how big the
 * data is. Furthermore, arbitrary data can be stored in the array.
 * .Pp
 * This array implementation is optimized for storage space and
 * appending. Deletions are expensive in that all the items of the list
 * above a deletion are moved down to fill the hole where the deletion
 * occurred. Insertions are also expensive in that all the elements
 * above the given index must be shifted up to make room for the new
 * element.
 * .Pp
 * Due to these design choices, this array implementation is best
 * suited to linear writing, and then linear or random reading.
 */

#include <stddef.h>
#include <stdarg.h>

/**
 * The functions in this API operate on an array structure which is
 * opaque to the caller.
 */
typedef struct Array Array;

/**
 * Allocate a new array. This function returns a pointer that can be
 * used with the other functions in this API, or NULL if there was an
 * error allocating memory for the array.
 */
extern Array * ArrayCreate(void);

/**
 * Deallocate an array. Note that this function does not free any of
 * the values stored in the array; it is the caller's job to manage the
 * memory for each item. Typically, the caller would iterate over all
 * the items in the array and free them before freeing the array.
 */
extern void ArrayFree(Array *);

/**
 * Get the size, in number of elements, of the given array.
 */
extern size_t ArraySize(Array *);

/**
 * Get the element at the specified index from the specified array.
 * This function will return NULL if the array is NULL, or the index
 * is out of bounds. Otherwise, it will return a pointer to a value
 * put into the array using
 * .Fn ArrayInsert
 * or
 * .Fn ArraySet .
 */
extern void * ArrayGet(Array *, size_t);

/**
 * Insert the specified element at the specified index in the specified
 * array. This function will shift the element currently at that index,
 * and any elements after it before inserting the given element.
 * .Pp
 * This function returns a boolean value indicating whether or not it
 * suceeded.
 */
extern int ArrayInsert(Array *, size_t, void *);

/**
 * Set the value at the specified index in the specified array to the
 * specified value. This function will return the old value at that
 * index, if any.
 */
extern void * ArraySet(Array *, size_t, void *);

/**
 * Append the specified element to the end of the specified array. This
 * function uses
 * .Fn ArrayInsert
 * under the hood to insert an element at the end. It thus has the same
 * return value as
 * .Fn ArrayInsert .
 */
extern int ArrayAdd(Array *, void *);

/**
 * Remove the element at the specified index from the specified array.
 * This function returns the element removed, if any.
 */
extern void * ArrayDelete(Array *, size_t);

/**
 * Sort the specified array using the specified sort function. The
 * sort function compares two elements. It takes two void pointers as
 * parameters, and returns an integer. The return value indicates to
 * the sorting algorithm how the elements relate to each other. A
 * return value of 0 indicates that the elements are identical. A
 * return value greater than 0 indicates that the first item is
 * ``bigger'' than the second item and should thus appear after it in
 * the array. A return value less than 0 indicates the opposite: the
 * second element should appear after the first in the array.
 */
extern void ArraySort(Array *, int (*) (void *, void *));

/**
 * If possible, reduce the amount of memory allocated to this array
 * by calling
 * .Fn Realloc
 * on the internal structure to perfectly fit the elements in the
 * array. This function is intended to be used by functions that return
 * relatively read-only arrays that will be long-lived.
 */
extern int ArrayTrim(Array *);

/**
 * Convert a variadic arguments list into an Array. In most cases, the
 * Array API is much easier to work with than
 * .Fn va_arg
 * and friends.
 */
extern Array * ArrayFromVarArgs(size_t, va_list);

/**
 * Duplicate an existing array. Note that arrays only hold pointers to
 * their data, not the data itself, so the duplicated array will point
 * to the same places in memory as the original array.
 */
extern Array * ArrayDuplicate(Array *);

#endif                             /* CYTOPLASM_ARRAY_H */
