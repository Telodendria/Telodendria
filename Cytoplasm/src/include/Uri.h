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
#ifndef CYTOPLASM_URI_H
#define CYTOPLASM_URI_H

/***
 * @Nm Uri
 * @Nd Parse a URI. Typically used to parse HTTP(s) URLs.
 * @Dd April 29 2023
 * @Xr Http
 *
 * .Nm
 * provides a simple mechanism for parsing URIs. This is an extremely
 * basic parser that (ab)uses
 * .Xr sscanf 3
 * to parse URIs, so it may not be the most reliable, but it should
 * work in most cases and on reasonable URIs that aren't too long, as
 * the _MAX definitions are modest.
 */

#define URI_PROTO_MAX 8
#define URI_HOST_MAX 128
#define URI_PATH_MAX 256

/**
 * The parsed URI is stored in this structure.
 */
typedef struct Uri
{
    char proto[URI_PROTO_MAX];
    char host[URI_HOST_MAX];
    char path[URI_PATH_MAX];
    unsigned short port;
} Uri;

/**
 * Parse a URI string into the Uri structure as described above, or
 * return NULL if there was a parsing error.
 */
extern Uri * UriParse(const char *);

/**
 * Free the memory associated with a Uri structure returned by
 * .Fn UriParse .
 */
extern void UriFree(Uri *);

#endif                             /* CYTOPLASM_URI_H */
