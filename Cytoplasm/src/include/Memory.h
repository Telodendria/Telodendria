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
#ifndef CYTOPLASM_MEMORY_H
#define CYTOPLASM_MEMORY_H

/***
 * @Nm Memory
 * @Nd Smart memory management.
 * @Dd January 9 2023
 *
 * .Nm
 * is an API that allows for smart memory management and profiling. It
 * wraps the standard library functions
 * .Xr malloc 3 ,
 * .Xr realloc 3 ,
 * and
 * .Xr free 3 ,
 * and offers identical semantics, while providing functionality that
 * the standard library doesn't have, such as getting statistics on the
 * total memory allocated on the heap, and getting the size of a block
 * given a pointer. Additionally, thanks to preprocessor macros, the
 * exact file and line number at which an allocation, re-allocation, or
 * free occured can be obtained given a pointer. Finally, all the
 * blocks allocated on the heap can be iterated and evaluated, and a
 * callback function can be executed every time a memory operation
 * occurs.
 * .Pp
 * In the future, this API could include a garbage collector that
 * automatically frees memory it detects as being no longer in use.
 * However, this feature does not yet exist.
 * .Pp
 * A number of macros are available, which make the
 * .Nm
 * API much easier to use. They are as follows:
 * .Bl -bullet -offset indent
 * .It
 * .Fn Malloc "x"
 * .It
 * .Fn Realloc "x" "y"
 * .It
 * .Fn Free "x"
 * .El
 * .Pp
 * These macros expand to
 * .Fn MemoryAllocate ,
 * .Fn MemoryReallocate ,
 * and
 * .Fn MemoryFree
 * with the second and third parameters set to __FILE__ and __LINE__.
 * This allows
 * .Nm
 * to be used exactly how the standard library functions would be
 * used. In fact, the functions to which these macros expand are not
 * intended to be used directly; for the best results, use these
 * macros.
 */
#include <stddef.h>

/**
 * These values are passed into the memory hook function to indicate
 * the action that just happened.
 */
typedef enum MemoryAction
{
    MEMORY_ALLOCATE,
    MEMORY_REALLOCATE,
    MEMORY_FREE,
    MEMORY_BAD_POINTER,
    MEMORY_CORRUPTED
} MemoryAction;

#define Malloc(x) MemoryAllocate(x, __FILE__, __LINE__)
#define Realloc(x, s) MemoryReallocate(x, s, __FILE__, __LINE__)
#define Free(x) MemoryFree(x, __FILE__, __LINE__)

/**
 * The memory information is opaque, but can be accessed using the
 * functions defined by this API.
 */
typedef struct MemoryInfo MemoryInfo;

/**
 * Allocate the specified number of bytes on the heap. This function
 * has the same semantics as
 * .Xr malloc 3 ,
 * except that it takes the file name and line number at which the
 * allocation occurred.
 */
extern void * MemoryAllocate(size_t, const char *, int);

/**
 * Change the size of the object pointed to by the given pointer
 * to the given number of bytes. This function has the same semantics
 * as
 * .Xr realloc 3 ,
 * except that it takes the file name and line number at which the
 * reallocation occurred.
 */
extern void * MemoryReallocate(void *, size_t, const char *, int);

/**
 * Free the memory at the given pointer. This function has the same
 * semantics as
 * .Xr free 3 ,
 * except that it takes the file name and line number at which the
 * free occurred.
 */
extern void MemoryFree(void *, const char *, int);

/**
 * Get the total number of bytes that the program has allocated on the
 * heap. This operation iterates over all heap allocations made with
 * .Fn MemoryAllocate
 * and then returns a total count, in bytes.
 */
extern size_t MemoryAllocated(void);

/**
 * Iterate over all heap allocations made with
 * .Fn MemoryAllocate
 * and call
 * .Fn MemoryFree
 * on them. This function immediately invalidates all pointers to
 * blocks on the heap, and any subsequent attempt to read or write to
 * data on the heap will result in undefined behavior. This is
 * typically called at the end of the program, just before exit.
 */
extern void MemoryFreeAll(void);

/**
 * Fetch information about an allocation. This function takes a raw
 * pointer, and if
 * . Nm
 * knows about the pointer, it returns a structure that can be used
 * to obtain information about the block of memory that the pointer
 * points to.
 */
extern MemoryInfo * MemoryInfoGet(void *);

/**
 * Get the size in bytes of the block of memory represented by the
 * specified memory info structure.
 */
extern size_t MemoryInfoGetSize(MemoryInfo *);

/**
 * Get the file name in which the block of memory represented by the
 * specified memory info structure was allocated.
 */
extern const char * MemoryInfoGetFile(MemoryInfo *);

/**
 * Get the line number on which the block of memory represented by the
 * specified memory info structure was allocated.
 */
extern int MemoryInfoGetLine(MemoryInfo *);

/**
 * Get a pointer to the block of memory represented by the specified
 * memory info structure.
 */
extern void * MemoryInfoGetPointer(MemoryInfo *);

/**
 * This function takes a pointer to a function that takes the memory
 * info structure, as well as a void pointer for caller-provided
 * arguments. It iterates over all the heap memory currently allocated
 * at the time of calling, executing the function on each allocation.
 */
extern void MemoryIterate(void (*) (MemoryInfo *, void *), void *);

/**
 * Specify a function to be executed whenever a memory operation
 * occurs. The MemoryAction argument specifies the operation that
 * occurred on the block of memory represented by the memory info
 * structure. The function also takes a void pointer to caller-provided
 * arguments.
 */
extern void MemoryHook(void (*) (MemoryAction, MemoryInfo *, void *), void *);

/**
 * The default memory hook, which has sane behavior and is installed
 * at runtime. This function does not use any memory on the heap,
 * except for the MemoryInfo passed to it, which it assumes to be
 * valid. Everything else happens on the stack only, to ensure that
 * the hook doesn't make any memory problems worse.
 */
extern void MemoryDefaultHook(MemoryAction, MemoryInfo *, void *);

/**
 * Read over the block of memory represented by the given memory info
 * structure and generate a hexadecimal and ASCII string for each
 * chunk of the block. This function takes a callback function that
 * takes the following parameters in order:
 * .Bl -bullet -offset indent
 * .It
 * The current offset from the beginning of the block of memory in
 * bytes.
 * .It
 * A null-terminated string containing the next 16 bytes of the block
 * encoded as space-separated hex values.
 * .It
 * A null-terminated string containing the ASCII representation of the
 * same 16 bytes of memory. This ASCII representation is safe to print
 * to a terminal or other text device, because non-printable characters
 * are encoded as a . (period).
 * .It
 * Caller-passed pointer.
 * .El
 */
extern void
MemoryHexDump(MemoryInfo *, void (*) (size_t, char *, char *, void *), void *);

#endif
