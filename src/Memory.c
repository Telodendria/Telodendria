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

    MemoryInfo *next;
    MemoryInfo *prev;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static MemoryInfo *lastAllocation = NULL;
static void (*hook) (MemoryAction, MemoryInfo *, void *) = NULL;
static void *hookArgs = NULL;

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
    a->next = NULL;
    a->prev = lastAllocation;

    if (lastAllocation)
    {
        lastAllocation->next = a;
    }

    lastAllocation = a;

    if (hook)
    {
        hook(MEMORY_ALLOCATE, a, hookArgs);
    }

    pthread_mutex_unlock(&lock);
    return p;
}

void *
MemoryReallocate(void *p, size_t size)
{
    MemoryInfo *a;
    void *new = NULL;

    pthread_mutex_lock(&lock);

    a = lastAllocation;
    while (a)
    {
        if (a->pointer == p)
        {
            new = realloc(p, size);
            if (new)
            {
                a->pointer = new;
                a->size = size;

                if (hook)
                {
                    hook(MEMORY_REALLOCATE, a, hookArgs);
                }
            }

            break;
        }

        a = a->prev;
    }

    pthread_mutex_unlock(&lock);

    return new;
}

void
MemoryFree(void *p)
{
    MemoryInfo *a;

    pthread_mutex_lock(&lock);

    a = lastAllocation;

    while (a)
    {
        if (a->pointer == p)
        {
            if (a->prev)
            {
                a->prev->next = a->next;
            }
            else
            {
                lastAllocation = a->next;
            }

            if (a->next)
            {
                a->next->prev = a->prev;
            }
            else
            {
                lastAllocation = a->prev;
            }

            if (hook)
            {
                hook(MEMORY_FREE, a, hookArgs);
            }

            free(a);
            free(p);

            break;
        }

        a = a->prev;
    }

    pthread_mutex_unlock(&lock);
}

size_t
MemoryAllocated(void)
{
    MemoryInfo *a;
    size_t total = 0;

    pthread_mutex_lock(&lock);

    a = lastAllocation;
    while (a)
    {
        total += a->size;
        a = a->prev;
    }

    pthread_mutex_unlock(&lock);

    return total;
}

void
MemoryFreeAll(void)
{
    MemoryInfo *a;

    pthread_mutex_lock(&lock);

    a = lastAllocation;
    while (a)
    {
        MemoryInfo *prev = a->prev;

        free(a->pointer);
        free(a);

        a = prev;
    }

    lastAllocation = NULL;

    pthread_mutex_unlock(&lock);
}

MemoryInfo *
MemoryInfoGet(void *p)
{
    MemoryInfo *a;

    pthread_mutex_lock(&lock);

    a = lastAllocation;
    while (a)
    {
        if (a->pointer == p)
        {
            break;
        }

        a = a->prev;
    }

    return a;
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
    MemoryInfo *a;

    pthread_mutex_lock(&lock);

    a = lastAllocation;
    while (a)
    {
        iterFunc(a, args);
        a = a->prev;
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
