/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net>
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

#ifndef TELODENDRIA_CANONICALJSON_H
#define TELODENDRIA_CANONICALJSON_H

/***
 * @Nm CanonicalJson
 * @Nd An extension of the JSON API that produces the Matrix spec's canonical JSON.
 * @Dd November 30 2022
 * @Xr Json
 *
 * This API is an extension of
 * .Xr json 3
 * that is specifically designed to produce the Matrix specification's
 * "canonical" JSON.
 * .Pp
 * Canonical JSON is defined as JSON that:
 * .Bl -bullet -offset indent
 * .It
 * Does not have any unecessary whitespace.
 * .It
 * Has all object key lexicographically sorted.
 * .It
 * Does not contain any floating point numerical values.
 * .El
 * .Pp
 * The regular JSON encoder has no such rules, because normally they
 * are not needed. However, Canonical JSON is needed in some cases to
 * consistently sign JSON objects.
 */

#include <stdio.h>

#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Stream.h>
#include <Cytoplasm/Json.h>

/**
 * Encode a JSON object following the rules of Canonical JSON. See the
 * documentation for
 * .Fn JsonEncode ,
 * documented in
 * .Xr Json 3
 * for more details on how JSON encoding operates. This function exists
 * as an alternative to
 * .Fn JsonEncode ,
 * but should not be preferred to it in most circumstances. It is a lot
 * more costly, as it must lexicographically sort all keys and strip
 * out float values. If at all possible, use
 * .Fn JsonEncode ,
 * because it is much cheaper both in terms of memory and CPU time.
 * .Pp
 * This function returns the number of bytes written to the
 * stream, just like
 * .Fn JsonEncode .
 */
extern int CanonicalJsonEncode(HashMap *, Stream *);

/**
 * Encode a JSON value following the rules of Canonical JSON.
 * See the documentation for
 * .Fn JsonEncodeValue ,
 * documented in
 * .Xr Json 3 .
 */
extern int CanonicalJsonEncodeValue(JsonValue *, Stream *);

#endif                             /* TELODENDRIA_CANONICALJSON_H */
