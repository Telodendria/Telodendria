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
 * included in all copies or substantial portions of the Software.
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
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOAT,
    JSON_BOOLEAN,
    JSON_NULL
} JsonType;

typedef struct JsonValue JsonValue;

extern JsonType
 JsonValueType(JsonValue * value);

extern JsonValue *
 JsonValueObject(HashMap * object);

extern HashMap *
 JsonValueAsObject(JsonValue * value);

extern JsonValue *
 JsonValueArray(Array * array);

extern Array *
 JsonValueAsArray(JsonValue * value);

extern JsonValue *
 JsonValueString(char *string);

extern JsonValue *
 JsonValueInteger(long integer);

extern JsonValue *
 JsonValueFloat(double floating);

extern JsonValue *
 JsonValueBoolean(int boolean);

extern JsonValue *
 JsonValueNull(void);

extern void
 JsonValueFree(JsonValue * value);

extern void
 JsonFree(HashMap * object);

extern void
 JsonEncodeString(const char *str, FILE * out);

extern void
 JsonEncodeValue(JsonValue * value, FILE * out);

extern int
 JsonEncode(HashMap * object, FILE * out);

extern HashMap *
 JsonDecode(FILE * in);

#endif
