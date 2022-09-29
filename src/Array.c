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
#include <Array.h>

#ifndef ARRAY_BLOCK
#define ARRAY_BLOCK 16
#endif

#include <stddef.h>
#include <stdlib.h>

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

    return ArrayInsert(array, value, array->size);
}

Array *
ArrayCreate(void)
{
    Array *array = malloc(sizeof(Array));

    if (!array)
    {
        return NULL;
    }

    array->size = 0;
    array->allocated = ARRAY_BLOCK;
    array->entries = malloc(sizeof(void *) * ARRAY_BLOCK);

    if (!array->entries)
    {
        free(array);
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
        free(array->entries);
        free(array);
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
ArrayInsert(Array * array, void *value, size_t index)
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

        array->entries = realloc(array->entries,
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

    array->entries = realloc(array->entries,
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

        ArrayQuickSort(array, low, pi - 1, compare);
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
    ArrayQuickSort(array, 0, array->size, compare);
}
