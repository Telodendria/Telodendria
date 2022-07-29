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

/*
 * Base64.h: An efficient base64 encoder and decoder that supports
 * both regular base64, and the Matrix spec's "unpadded base64."
 */
#ifndef TELODENDRIA_BASE64_H
#define TELODENDRIA_BASE64_H

#include <stddef.h>

/*
 * Compute the encoded size, including padding, of an input with the
 * provided size.
 *
 * Params:
 *
 *   (size_t) The size of the input data to be encoded.
 *
 * Return: The size of the string needed to hold the data in its
 * encoded form. Note that base64 is not compression; base64 strings
 * are actually 25% larger than the unencoded input.
 */
extern size_t
 Base64EncodedSize(size_t);

/*
 * Compute the decoded size of the provide base64 input and length.
 *
 * Note that both the size and the actual base64 string itself is
 * needed for this computation, unlike Base64EncodedSize(). This is
 * because base64 strings are padded, and that padding is used in the
 * calculations.
 *
 * Params:
 *
 *   (const char *) A padded base64 string. If you are dealing with
 *                  potentially user-provided base64, you should call
 *                  Base64Pad() on it to normalize it before computing
 *                  the decoded size.
 *   (size_t) The length of the base64 string. Instead of scanning the
 *            string for a null terminator, and then working backwards,
 *            the length of the string must be passed here.
 *
 * Return: The number of bytes that can be decoded from this base64
 * string. Note that this will be smaller than the length of the base64
 * string because base64 is larger than the unencoded form.
 */
extern size_t
 Base64DecodedSize(const char *, size_t);

/*
 * Copy the given input string to a new string, base64 encoding it in
 * the process. This function will produce standard padded base64. If
 * you want unpadded base64, call Base64Unpad() on the return value
 * of this function.
 *
 * Params:
 *
 *   (const char *) The raw, unencoded input to be encoded as base64.
 *   (size_t)       The length of the unencoded input string.
 *
 * Return: A new string, allocated on the heap, that holds the base64
 * representation of the input. This string must be free()-ed when it
 * is no longer needed. If the allocation of the proper size fails,
 * or the input is inaccessible, then this function will return NULL.
 */
extern char *
 Base64Encode(const char *, size_t);

/*
 * Decode a standard padded base64 string. This function expects that
 * the input will be padded, so if you are recieving untrusted input,
 * you should run Base64Pad() on it before attempting to decode it.
 *
 * Params:
 *
 *   (const char *) The base64 string to decode.
 *   (size_t)       The length of the base64 string to decode.
 *
 * Return: A new string, allocated on the heap, that contains the
 * decoded string, or NULL if a decoding error occurred.
 */
extern char *
 Base64Decode(const char *, size_t);

/*
 * Remove the padding from a base64 string. This is to implement the
 * Matrix spec's "unpadded base64" functionality. When base64 strings
 * are sent to other servers and clients, their padding must be
 * stripped.
 *
 * Params:
 *
 *   (char *) The base64 string to remove padding from. Note that
 *            this string is modified in place.
 *   (size_t) The length of the provided base64 string.
 *
 */
extern void
 Base64Unpad(char *, size_t);

/*
 * Pad a base64 string in place. This is to implement the Matrix spec's
 * "unpadded base64." As we will most likely be getting unpadded base64
 * from clients and other servers, we should pad it before attempting
 * to decode it.
 *
 * I technically could have just had the decoder accept unpadded as
 * well as padded strings, but Matrix is the only thing I know of that
 * actually makes "unpadded" base64 a thing, so I thought it best to
 * make it clear in this library that unpadded base64 is an extension,
 * not the norm.
 *
 * Params:
 *
 *   (char **) A pointer to a base64 string pointer. The reason we
 *             take a pointer pointer is because the string may need
 *             to be reallocated, as characters may be added to the
 *             end of it. If the string is reallocated, then the
 *             passed pointer must be updated. If the string is not
 *             reallocated, the original pointer is not touched.
 *   (size_t)  The length of the given base64 string.
 *
 * Return: Whether or not the pad operation was successful. This
 * function will fail if a larger string cannot be allocated when it
 * is needed. Note that not all cases require the string to be
 * reallocated.
 */
extern int
 Base64Pad(char **, size_t);

#endif
