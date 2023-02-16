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

#ifndef TELODENDRIA_JSON_H
#define TELODENDRIA_JSON_H

#include <HashMap.h>
#include <Array.h>

#include <stdio.h>
#include <stddef.h>

typedef enum JsonType
{
    JSON_NULL,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOAT,
    JSON_BOOLEAN
} JsonType;

typedef struct JsonValue JsonValue;

extern JsonType
 JsonValueType(JsonValue *);

extern JsonValue *
 JsonValueObject(HashMap *);

extern HashMap *
 JsonValueAsObject(JsonValue *);

extern JsonValue *
 JsonValueArray(Array *);

extern Array *
 JsonValueAsArray(JsonValue *);

extern JsonValue *
 JsonValueString(char *);

extern char *
 JsonValueAsString(JsonValue *);

extern JsonValue *
 JsonValueInteger(long);

extern long
 JsonValueAsInteger(JsonValue *);

extern JsonValue *
 JsonValueFloat(double);

extern double
 JsonValueAsFloat(JsonValue *);

extern JsonValue *
 JsonValueBoolean(int);

extern int
 JsonValueAsBoolean(JsonValue *);

extern JsonValue *
 JsonValueNull(void);

extern void
 JsonValueFree(JsonValue *);

extern void
 JsonFree(HashMap *);

extern void
 JsonEncodeString(const char *, FILE *);

extern void
 JsonEncodeValue(JsonValue * value, FILE * out);

extern int
 JsonEncode(HashMap *, FILE *);

extern HashMap *
 JsonDecode(FILE *);

extern JsonValue *
 JsonGet(HashMap *, size_t, ...);

extern JsonValue *
 JsonSet(HashMap *, JsonValue *, size_t, ...);

extern int
 JsonCreate(HashMap *, JsonValue *, size_t, ...);

#endif                             /* TELODENDRIA_JSON_H */
