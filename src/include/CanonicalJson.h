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
 * CanonicalJson.h: An expansion of the JSON encoding functionality
 * that is specifically designed to produce the Matrix spec's
 * "canonical" JSON.
 *
 * Canonical JSON is defined as JSON that:
 *
 *   - Does not have any unecessary whitespace.
 *   - Has all object keys lexicographically sorted.
 *   - Does not contain any float values.
 *
 * The regular JSON encoder has no such rules, because normally they
 * are not needed. However, Canonical JSON is needed to be able to
 * sign JSON objects in a consistent way.
 */
#ifndef TELODENDRIA_CANONICALJSON_H
#define TELODENDRIA_CANONICALJSON_H

#include <stdio.h>
#include <HashMap.h>

/*
 * Encode a JSON object following the rules of canonical JSON. See
 * JsonEncode() for more details on how JSON encoding operates.
 *
 * This function exists as an alternative to JsonEncode(), but should
 * not be preferred to JsonEncode() in normal circumstances. It is
 * a lot more costly, as it must lexicographically sort all keys and
 * strip out float values. If at all possible, use JsonEncode(),
 * because it is much cheaper in terms of memory and CPU time.
 *
 * Params:
 *
 *   (HashMap *) The JSON object to encode. Note that all values must
 *               be JsonValues.
 *   (FILE *)    The output stream to write the JSON object to.
 *
 * Return: Whether or not the JSON encoding was successful. This
 * function may fail if NULL was given for any parameter.
 */
extern int
 CanonicalJsonEncode(HashMap *, FILE *);

/*
 * Encode the JSON object to a string. The regular JSON encoding
 * library doesn't have a way to send JSON to strings, because there's
 * absolutely no reason to handle JSON strings. However, the sole
 * reason canonical JSON exists is so that JSON objects can be signed.
 * Thus, you need a string to pass to the signing function.
 *
 * Params:
 *
 *   (HashMap *) The JSON object to encode. Note that all values must
 *               be JsonValues.
 *
 * Return: A string containing the canonical JSON representation of
 * the given object, or NULL if the encoding failed.
 */
extern char *
 CanonicalJsonEncodeToString(HashMap *);

#endif                             /* TELODENDRIA_CANONICALJSON_H */
