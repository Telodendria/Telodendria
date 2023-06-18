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
#include <Routes.h>

#include <string.h>

#include <HashMap.h>
#include <Memory.h>
#include <User.h>
#include <Json.h>
#include <Str.h>

static HashMap *
ValidateRoomFilter(HashMap *json)
{
    return NULL;
}

static HashMap *
ValidateEventFields(Array *fields)
{
    return NULL;
}

static HashMap *
ValidateEventFormat(char *fmt)
{
    return NULL;
}

static HashMap *
ValidateEventFilter(HashMap *json)
{
    JsonValue *val;

    val = HashMapGet(json, "limit");
    if (val)
    {
        if (JsonValueType(val) == JSON_INTEGER)
        {
            long limit = JsonValueAsInteger(val);
            if (limit <= 0 || limit > 100)
            {
                return MatrixErrorCreate(M_BAD_JSON);
            }
        }
        else
        {
            return MatrixErrorCreate(M_BAD_JSON);
        }
    }

    return NULL;
}

HashMap *
FilterValidate(HashMap *json)
{
    JsonValue *val;
    HashMap *response = NULL;

#define VALIDATE(key, type, func, param) \
    val = HashMapGet(json, key); \
    if (val) \
    { \
        if (JsonValueType(val) == type) \
        { \
            response = func(param); \
            if (response) \
            { \
                goto finish; \
            } \
        } \
        else \
        { \
            return MatrixErrorCreate(M_BAD_JSON); \
        } \
    }

    VALIDATE("account_data", JSON_OBJECT, ValidateEventFilter, JsonValueAsObject(val));
    VALIDATE("event_fields", JSON_ARRAY, ValidateEventFields, JsonValueAsArray(val));
    VALIDATE("event_format", JSON_STRING, ValidateEventFormat, JsonValueAsString(val));
    VALIDATE("presence", JSON_OBJECT, ValidateEventFilter, JsonValueAsObject(val));
    VALIDATE("room", JSON_OBJECT, ValidateRoomFilter, JsonValueAsObject(val));

#undef VALIDATE

finish:
    return response;
}

