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

#ifndef CYTOPLASM_BASE64_H
#define CYTOPLASM_BASE64_H

/***
 * @Nm Base64
 * @Nd A simple base64 encoder/decoder with unpadded base64 support.
 * @Dd September 30 2022
 * @Xr Sha2
 *
 * This is an efficient yet simple base64 encoding and decoding API
 * that supports regular base64, as well as the Matrix specification's
 * extension to base64, called ``unpadded base64.'' This API provides
 * the ability to convert between the two, instead of just implementing
 * unpadded base64.
 */

#include <stddef.h>

/**
 * This function computes the amount of bytes needed to store a message
 * of the specified number of bytes as base64.
 */
extern size_t
 Base64EncodedSize(size_t);

/**
 * This function computes the amount of bytes needed to store a decoded
 * representation of the encoded message. It takes a pointer to the
 * encoded string because it must read a few bytes off the end in order
 * to accurately compute the size.
 */
extern size_t
 Base64DecodedSize(const char *, size_t);

/**
 * Encode the specified number of bytes from the specified buffer as
 * base64. This function returns a string on the heap that should be
 * freed with
 * .Fn Free ,
 * or NULL if a memory allocation error ocurred.
 */
extern char *
 Base64Encode(const char *, size_t);

/**
 * Decode the specified number of bytes from the specified buffer of
 * base64. This function returns a string on the heap that should be
 * freed with
 * .Fn Free ,
 * or NULL if a memory allocation error occured.
 */
extern char *
 Base64Decode(const char *, size_t);

/**
 * Remove the padding from a specified base64 string. This function
 * modifies the specified string in place. It thus has no return value
 * because it cannot fail. If the passed pointer is invalid, the
 * behavior is undefined.
 */
extern void
 Base64Unpad(char *, size_t);

/**
 * Add padding to an unpadded base64 string. This function takes a
 * pointer to a pointer because it may be necessary to grow the memory
 * allocated to the string. This function returns a boolean value
 * indicating whether the pad operation was successful. In practice,
 * this means it will only fail if a bigger string is necessary, but it
 * could not be automatically allocated on the heap.
 */
extern int
 Base64Pad(char **, size_t);

#endif                             /* CYTOPLASM_BASE64_H */
