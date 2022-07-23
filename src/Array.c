#include <Array.h>

#ifndef ARRAY_BLOCK
#define ARRAY_BLOCK 16
#endif

#include <stddef.h>
#include <stdlib.h>

struct Array {
    void **entries; /* An array of void pointers, to store any data */
    size_t allocated; /* Elements allocated on the heap */
    size_t size; /* Elements actually filled */
};

int
ArrayAdd(Array *array, void *value)
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
ArrayDelete(Array *array, size_t index)
{
    size_t i;
    void *element;

    if (!array)
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
ArrayFree(Array *array)
{
    if (array)
    {
        free(array->entries);
        free(array);
    }
}

void *
ArrayGet(Array *array, size_t index)
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
ArrayInsert(Array *array, void *value, size_t index)
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
ArraySize(Array *array)
{
    if (!array)
    {
        return 0;
    }

    return array->size;
}

int
ArrayTrim(Array *array)
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
