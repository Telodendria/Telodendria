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

#ifndef CYTOPLASM_SHA_H
#define CYTOPLASM_SHA_H

/***
 * @Nm Sha
 * @Nd A simple implementation of a few SHA hashing functions.
 * @Dd December 19 2022
 * @Xr Memory Base64
 *
 * This API defines simple functions for computing SHA hashes.
 * At the moment, it only defines
 * .Fn Sha256
 * and
 * .Fn Sha1 ,
 * which compute the SHA-256 and SHA-1 hashes of the given C string,
 * respectively. It is not trivial to implement SHA-512 in ANSI C
 * due to the lack of a 64-bit integer type, so that hash
 * function has been omitted.
 */

/**
 * This function takes a pointer to a NULL-terminated C string, and
 * returns a NULL-terminated byte buffer allocated on the heap using
 * the Memory API, or NULL if there was an error allocating memory.
 * The returned byte buffer should be freed when it is no longer
 * needed. It is important to note that the returned buffer is not
 * a printable string; to get a printable string, use 
 * .Fn ShaToHex .
 */
extern unsigned char * Sha256(char *);

/**
 * This function takes a pointer to a NULL-terminated C string, and
 * returns a NULL-terminated byte buffer allocated on the heap using
 * the Memory API, or NULL if there was an error allocating memory.
 * The returned byte buffer should be freed when it is no longer
 * needed. It is important to note that the returned buffer is not
 * a printable string; to get a printable string, use 
 * .Fn ShaToHex .
 */
extern unsigned char * Sha1(char *);

/**
 * Convert a SHA byte buffer into a hex string. These hex strings
 * are typically what is transmitted, stored, and compared, however
 * there may be times when it is necessary to work with the raw
 * bytes directly, which is why the conversion to a hex string is
 * a separate step.
 */
extern char * ShaToHex(unsigned char *);

#endif                             /* CYTOPLASM_SHA_H */
