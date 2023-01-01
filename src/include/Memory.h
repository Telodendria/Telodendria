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
#ifndef TELODENDRIA_MEMORY_H
#define TELODENDRIA_MEMORY_H

#include <stddef.h>

typedef enum MemoryAction
{
    MEMORY_ALLOCATE,
    MEMORY_REALLOCATE,
    MEMORY_FREE,
    MEMORY_BAD_POINTER
} MemoryAction;

#define Malloc(x) MemoryAllocate(x, __FILE__, __LINE__)
#define Realloc(x, s) MemoryReallocate(x, s, __FILE__, __LINE__)
#define Free(x) MemoryFree(x, __FILE__, __LINE__)

typedef struct MemoryInfo MemoryInfo;

extern void *MemoryAllocate(size_t, const char *, int);
extern void *MemoryReallocate(void *, size_t, const char *, int);
extern void MemoryFree(void *, const char *, int);

extern size_t MemoryAllocated(void);
extern void MemoryFreeAll(void);

extern MemoryInfo *MemoryInfoGet(void *);

extern size_t MemoryInfoGetSize(MemoryInfo *);
extern const char *MemoryInfoGetFile(MemoryInfo *);
extern int MemoryInfoGetLine(MemoryInfo *);
extern void *MemoryInfoGetPointer(MemoryInfo *);

extern void MemoryIterate(void (*) (MemoryInfo *, void *), void *);

extern void MemoryHook(void (*) (MemoryAction, MemoryInfo *, void *), void *);

extern void MemoryHexDump(MemoryInfo *, void (*) (size_t, char *, char *, void *), void *);

#endif
