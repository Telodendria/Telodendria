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
#include <Memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct MemoryInfo
{
    size_t size;
    const char *file;
    int line;
    void *pointer;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static void (*hook) (MemoryAction, MemoryInfo *, void *) = NULL;
static void *hookArgs = NULL;

static MemoryInfo **allocations = NULL;
static size_t allocationsSize = 0;
static size_t allocationsLen = 0;

static size_t
MemoryHash(void *p)
{
    return (((size_t) p) >> 2 * 7) % allocationsSize;
}

static int
MemoryInsert(MemoryInfo * a)
{
    size_t hash;

    if (!allocations)
    {
        allocationsSize = 64;
        allocations = calloc(allocationsSize, sizeof(void *));
        if (!allocations)
        {
            return 0;
        }
    }

    if ((allocationsLen + 1) >= (0.75 * allocationsSize))
    {
        size_t i;
        size_t tmpAllocationsSize = allocationsSize;
        MemoryInfo **tmpAllocations;

        allocationsSize *= 2;
        tmpAllocations = calloc(allocationsSize, sizeof(void *));

        if (!tmpAllocations)
        {
            return 0;
        }

        for (i = 0; i < tmpAllocationsSize; i++)
        {
            if (allocations[i])
            {
                hash = MemoryHash(allocations[i]->pointer);

                while (tmpAllocations[hash])
                {
                    hash = (hash + 1) % allocationsSize;
                }

                tmpAllocations[hash] = allocations[i];
            }
        }

        free(allocations);
        allocations = tmpAllocations;
    }

    hash = MemoryHash(a->pointer);

    while (allocations[hash])
    {
        hash = (hash + 1) % allocationsSize;
    }

    allocations[hash] = a;
    allocationsLen++;

    return 1;
}

static void
MemoryDelete(MemoryInfo * a)
{
    size_t hash = MemoryHash(a->pointer);
    size_t count = 0;

    while (count <= allocationsSize)
    {
        if (allocations[hash] && allocations[hash] == a)
        {
            allocations[hash] = NULL;
            allocationsLen--;
            return;
        }
        else
        {
            hash = (hash + 1) % allocationsSize;
            count++;
        }
    }
}

void *
MemoryAllocate(size_t size, const char *file, int line)
{
    void *p;
    MemoryInfo *a;

    pthread_mutex_lock(&lock);

    p = malloc(size);
    if (!p)
    {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    a = malloc(sizeof(MemoryInfo));
    if (!a)
    {
        free(p);
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    a->size = size;
    a->file = file;
    a->line = line;
    a->pointer = p;

    if (!MemoryInsert(a))
    {
        free(a);
        free(p);
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    if (hook)
    {
        hook(MEMORY_ALLOCATE, a, hookArgs);
    }

    pthread_mutex_unlock(&lock);
    return p;
}

void *
MemoryReallocate(void *p, size_t size, const char *file, int line)
{
    MemoryInfo *a;
    void *new = NULL;

    if (!p)
    {
        return MemoryAllocate(size, file, line);
    }

    a = MemoryInfoGet(p);
    if (a)
    {
        pthread_mutex_lock(&lock);
        new = realloc(a->pointer, size);
        if (new)
        {
            MemoryDelete(a);
            a->size = size;
            a->file = file;
            a->line = line;

            a->pointer = new;
            MemoryInsert(a);

            if (hook)
            {
                hook(MEMORY_REALLOCATE, a, hookArgs);
            }

        }
        pthread_mutex_unlock(&lock);
    }
    else if (hook)
    {
        a = malloc(sizeof(MemoryInfo));
        if (a)
        {
            a->size = 0;
            a->file = file;
            a->line = line;
            a->pointer = p;
            hook(MEMORY_BAD_POINTER, a, hookArgs);
            free(a);
        }
    }


    return new;
}

void
MemoryFree(void *p, const char *file, int line)
{
    MemoryInfo *a;

    if (!p)
    {
        return;
    }

    a = MemoryInfoGet(p);
    if (a)
    {
        pthread_mutex_lock(&lock);
        if (hook)
        {
            a->file = file;
            a->line = line;
            hook(MEMORY_FREE, a, hookArgs);
        }
        MemoryDelete(a);
        free(a->pointer);
        free(a);

        pthread_mutex_unlock(&lock);
    }
    else if (hook)
    {
        a = malloc(sizeof(MemoryInfo));
        if (a)
        {
            a->file = file;
            a->line = line;
            a->size = 0;
            a->pointer = p;
            hook(MEMORY_BAD_POINTER, a, hookArgs);
            free(a);
        }
    }
}

size_t
MemoryAllocated(void)
{
    size_t i;
    size_t total = 0;

    pthread_mutex_lock(&lock);

    for (i = 0; i < allocationsSize; i++)
    {
        if (allocations[i])
        {
            total += allocations[i]->size;
        }
    }

    pthread_mutex_unlock(&lock);

    return total;
}

void
MemoryFreeAll(void)
{
    size_t i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < allocationsSize; i++)
    {
        if (allocations[i])
        {
            free(allocations[i]->pointer);
            free(allocations[i]);
        }
    }

    free(allocations);
    allocations = NULL;
    allocationsSize = 0;
    allocationsLen = 0;

    pthread_mutex_unlock(&lock);
}

MemoryInfo *
MemoryInfoGet(void *p)
{
    size_t hash, count;

    pthread_mutex_lock(&lock);

    hash = MemoryHash(p);

    count = 0;
    while (count <= allocationsSize)
    {
        if (!allocations[hash] || allocations[hash]->pointer != p)
        {
            hash = (hash + 1) % allocationsSize;
            count++;
        }
        else
        {
            pthread_mutex_unlock(&lock);
            return allocations[hash];
        }
    }

    pthread_mutex_unlock(&lock);
    return NULL;
}

size_t
MemoryInfoGetSize(MemoryInfo * a)
{
    if (!a)
    {
        return 0;
    }

    return a->size;
}

const char *
MemoryInfoGetFile(MemoryInfo * a)
{
    if (!a)
    {
        return NULL;
    }

    return a->file;
}

int
MemoryInfoGetLine(MemoryInfo * a)
{
    if (!a)
    {
        return -1;
    }

    return a->line;
}

void *
MemoryInfoGetPointer(MemoryInfo * a)
{
    if (!a)
    {
        return NULL;
    }

    return a->pointer;
}

void
 MemoryIterate(void (*iterFunc) (MemoryInfo *, void *), void *args)
{
    size_t i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < allocationsSize; i++)
    {
        if (allocations[i])
        {
            iterFunc(allocations[i], args);
        }
    }

    pthread_mutex_unlock(&lock);
}

void
 MemoryHook(void (*memHook) (MemoryAction, MemoryInfo *, void *), void *args)
{
    pthread_mutex_lock(&lock);
    hook = memHook;
    hookArgs = args;
    pthread_mutex_unlock(&lock);
}
