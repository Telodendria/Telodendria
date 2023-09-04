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
#include <Array.h>

#ifndef ARRAY_BLOCK
#define ARRAY_BLOCK 16
#endif

#include <stddef.h>
#include <Memory.h>

struct Array
{
    void **entries;                /* An array of void pointers, to
                                    * store any data */
    size_t allocated;              /* Elements allocated on the heap */
    size_t size;                   /* Elements actually filled */
};

int
ArrayAdd(Array * array, void *value)
{
    if (!array)
    {
        return 0;
    }

    return ArrayInsert(array, array->size, value);
}

Array *
ArrayCreate(void)
{
    Array *array = Malloc(sizeof(Array));

    if (!array)
    {
        return NULL;
    }

    array->size = 0;
    array->allocated = ARRAY_BLOCK;
    array->entries = Malloc(sizeof(void *) * ARRAY_BLOCK);

    if (!array->entries)
    {
        Free(array);
        return NULL;
    }

    return array;
}

void *
ArrayDelete(Array * array, size_t index)
{
    size_t i;
    void *element;

    if (!array || array->size <= index)
    {
        return NULL;
    }

    element = array->entries[index];

    for (i = index; i < array->size - 1; i++)
    {
        array->entries[i] = array->entries[i + 1];
    }

    array->size--;

    return element;
}

void
ArrayFree(Array * array)
{
    if (array)
    {
        Free(array->entries);
        Free(array);
    }
}

void *
ArrayGet(Array * array, size_t index)
{
    if (!array)
    {
        return NULL;
    }

    if (index >= array->size)
    {
        return NULL;
    }

    return array->entries[index];
}


extern int
ArrayInsert(Array * array, size_t index, void *value)
{
    size_t i;

    if (!array || !value || index > array->size)
    {
        return 0;
    }

    if (array->size >= array->allocated)
    {
        void **tmp;
        size_t newSize = array->allocated + ARRAY_BLOCK;

        tmp = array->entries;

        array->entries = Realloc(array->entries,
                                 sizeof(void *) * newSize);

        if (!array->entries)
        {
            array->entries = tmp;
            return 0;
        }

        array->allocated = newSize;
    }

    for (i = array->size; i > index; i--)
    {
        array->entries[i] = array->entries[i - 1];
    }

    array->size++;

    array->entries[index] = value;

    return 1;
}

extern void *
ArraySet(Array * array, size_t index, void *value)
{
    void *oldValue;

    if (!value)
    {
        return ArrayDelete(array, index);
    }

    if (!array)
    {
        return NULL;
    }

    if (index >= array->size)
    {
        return NULL;
    }

    oldValue = array->entries[index];
    array->entries[index] = value;

    return oldValue;
}

size_t
ArraySize(Array * array)
{
    if (!array)
    {
        return 0;
    }

    return array->size;
}

int
ArrayTrim(Array * array)
{
    void **tmp;

    if (!array)
    {
        return 0;
    }

    tmp = array->entries;

    array->entries = Realloc(array->entries,
                             sizeof(void *) * array->size);

    if (!array->entries)
    {
        array->entries = tmp;
        return 0;
    }

    return 1;
}

static void
ArraySwap(Array * array, size_t i, size_t j)
{
    void *p = array->entries[i];

    array->entries[i] = array->entries[j];
    array->entries[j] = p;
}

static size_t
ArrayPartition(Array * array, size_t low, size_t high, int (*compare) (void *, void *))
{
    void *pivot = array->entries[high];
    size_t i = low - 1;
    size_t j;

    for (j = low; j <= high - 1; j++)
    {
        if (compare(array->entries[j], pivot) < 0)
        {
            i++;
            ArraySwap(array, i, j);
        }
    }
    ArraySwap(array, i + 1, high);
    return i + 1;
}

static void
ArrayQuickSort(Array * array, size_t low, size_t high, int (*compare) (void *, void *))
{
    if (low < high)
    {
        size_t pi = ArrayPartition(array, low, high, compare);

        ArrayQuickSort(array, low, pi ? pi - 1 : 0, compare);
        ArrayQuickSort(array, pi + 1, high, compare);
    }
}

void
ArraySort(Array * array, int (*compare) (void *, void *))
{
    if (!array)
    {
        return;
    }
    ArrayQuickSort(array, 0, array->size - 1, compare);
}

Array *
ArrayUnique(Array * array, int (*compare) (void *, void *))
{
    Array *ret;

    size_t i;

    if (!array)
    {
        return NULL;
    }

    ret = ArrayDuplicate(array);
    if (!ret)
    {
        return NULL;
    }

    if (ArraySize(ret) == 1)
    {
        /* There can't be any duplicates when there's only 1 value */
        return ret;
    }

    ArraySort(ret, compare);

    for (i = 1; i < ArraySize(ret); i++)
    {
        void *cur = ret->entries[i];
        void *prev = ret->entries[i - 1];

        if (compare(cur, prev) == 0)
        {
            /* Remove the duplicate, and put i back where it was. */
            ArrayDelete(ret, i--);
        }
    }

    ArrayTrim(ret);

    return ret;
}


/* Even though the following operations could be done using only the
 * public Array API defined above, I opted for low-level struct
 * manipulation because it allows much more efficient copying; we only
 * allocate what we for sure need upfront, and don't have to
 * re-allocate during the operation. */

Array *
ArrayReverse(Array * array)
{
    Array *ret;

    size_t i;
    size_t size;

    if (!array)
    {
        return NULL;
    }

    if (!array->size)
    {
        return ArrayCreate();
    }

    ret = Malloc(sizeof(Array));

    size = array->size;

    ret->size = size;
    ret->allocated = size;
    ret->entries = Malloc(sizeof(void *) * size);

    if (!ret->entries)
    {
        Free(ret);
        return NULL;
    }

    for (i = 0; i < size; i++)
    {
        ret->entries[i] = array->entries[size - i - 1];
    }

    return ret;
}

Array *
ArrayFromVarArgs(size_t n, va_list ap)
{
    size_t i;
    Array *arr = Malloc(sizeof(Array));

    if (!arr)
    {
        return NULL;
    }

    arr->size = n;
    arr->allocated = n;
    arr->entries = Malloc(sizeof(void *) * arr->allocated);

    if (!arr->entries)
    {
        Free(arr);
        return NULL;
    }

    for (i = 0; i < n; i++)
    {
        arr->entries[i] = va_arg(ap, void *);
    }

    return arr;
}

Array *
ArrayDuplicate(Array * arr)
{
    size_t i;
    Array *arr2 = Malloc(sizeof(Array));

    if (!arr2)
    {
        return NULL;
    }

    arr2->size = arr->size;
    arr2->allocated = arr->size;
    arr2->entries = Malloc(sizeof(void *) * arr->allocated);

    if (!arr2->entries)
    {
        Free(arr2);
        return NULL;
    }

    for (i = 0; i < arr2->size; i++)
    {
        arr2->entries[i] = arr->entries[i];
    }

    return arr2;
}
