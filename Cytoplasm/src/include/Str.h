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
#ifndef CYTOPLASM_STR_H
#define CYTOPLASM_STR_H

/***
 * @Nm Str
 * @Nd Functions for creating and manipulating strings.
 * @Dd February 15 2023
 * @Xr Memory
 *
 * .Nm
 * provides string-related functions. It is called
 * .Nm ,
 * not String, because some platforms (Windows) do not have
 * case-sensitive filesystems, which poses a problem since
 * .Pa string.h
 * is a standard library header.
 */

#include <stddef.h>

/**
 * Take a UTF-8 codepoint and encode it into a string buffer containing
 * between 1 and 4 bytes. The string buffer is allocated on the heap,
 * so it should be freed when it is no longer needed.
 */
extern char * StrUtf8Encode(unsigned long);

/**
 * Duplicate a null-terminated string, returning a new string on the
 * heap. This is useful when a function takes in a string that it needs
 * to store for long amounts of time, even perhaps after the original
 * string is gone.
 */
extern char * StrDuplicate(const char *);

/**
 * Extract part of a null-terminated string, returning a new string on
 * the heap containing only the requested subsection. Like the
 * substring functions included with most programming languages, the
 * starting index is inclusive, and the ending index is exclusive.
 */
extern char * StrSubstr(const char *, size_t, size_t);

/**
 * A varargs function that takes a number of null-terminated strings
 * specified by the first argument, and returns a new string that
 * contains their concatenation. It works similarly to
 * .Xr strcat 3 ,
 * but it takes care of allocating memory big enough to hold all the
 * strings. Any string in the list may be NULL. If a NULL pointer is
 * passed, it is treated like an empty string.
 */
extern char * StrConcat(size_t,...);

/**
 * Return a boolean value indicating whether or not the null-terminated
 * string consists only of blank characters, as determined by
 * .Xr isblank 3 .
 */
extern int StrBlank(const char *str);

/**
 * Generate a string of the specified length, containing random
 * lowercase and uppercase letters.
 */
extern char * StrRandom(size_t);

/**
 * Convert the specified integer into a string, returning the string
 * on the heap, or NULL if there was a memory allocation error. The
 * returned string should be freed by the caller after it is no longer
 * needed.
 */
extern char * StrInt(long);

/**
 * Compare two strings and determine whether or not they are equal.
 * This is the most common use case of strcmp() in Cytoplasm, but
 * strcmp() doesn't like NULL pointers, so these have to be checked
 * explicitly and can cause problems if they aren't. This function,
 * on the other hand, makes NULL pointers special cases. If both
 * arguments are NULL, then they are considered equal. If only one
 * argument is NULL, they are considered not equal. Otherwise, if
 * no arguments are NULL, a regular strcmp() takes place and this
 * function returns a boolean value indicating whether or not
 * strcmp() returned 0.
 */
extern int StrEquals(const char *, const char *);

#endif                             /* CYTOPLASM_STR_H */
