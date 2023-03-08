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
#include <CanonicalJson.h>

#include <HashMap.h>
#include <Array.h>
#include <Json.h>

#include <stdio.h>
#include <string.h>

int
CanonicalJsonKeyCompare(void *k1, void *k2)
{
    return strcmp((char *) k1, (char *) k2);
}

static void
CanonicalJsonEncodeValue(JsonValue * value, FILE * out)
{
    Array *arr;
    size_t i, len;

    /* Override object type to encode using the canonical functions */
    switch (JsonValueType(value))
    {
        case JSON_OBJECT:
            CanonicalJsonEncode(JsonValueAsObject(value), out);
            break;
        case JSON_ARRAY:
            arr = JsonValueAsArray(value);
            len = ArraySize(arr);

            fputc('[', out);

            for (i = 0; i < len; i++)
            {
                JsonValue *aVal = ArrayGet(arr, i);

                if (JsonValueType(aVal) == JSON_FLOAT)
                {
                    /* See comment in CanonicalJsonEncode() */
                    continue;
                }

                CanonicalJsonEncodeValue(aVal, out);
                if (i < len - 1)
                {
                    fputc(',', out);
                }
            }

            fputc(']', out);
            break;
        default:
            JsonEncodeValue(value, out, JSON_DEFAULT);
            break;
    }
}

int
CanonicalJsonEncode(HashMap * object, FILE * out)
{
    char *key;
    JsonValue *value;
    Array *keys;
    size_t i;
    size_t keyCount;

    if (!object || !out)
    {
        return 0;
    }

    keys = ArrayCreate();
    if (!keys)
    {
        return 0;
    }

    while (HashMapIterate(object, &key, (void **) &value))
    {
        ArrayAdd(keys, key);
    }

    ArraySort(keys, CanonicalJsonKeyCompare);

    fputc('{', out);

    keyCount = ArraySize(keys);
    for (i = 0; i < keyCount; i++)
    {
        key = (char *) ArrayGet(keys, i);
        value = (JsonValue *) HashMapGet(object, key);

        if (JsonValueType(value) == JSON_FLOAT)
        {
            /*
             * "INFO: Float values are not permitted by this encoding."
             *
             * The spec doesn't say how a canonical JSON generator should
             * handle float values, but given that it is highly unlikely
             * that we will ever be using float values, it shouldn't be
             * an issue if we just skip keys that have float values
             * altogether.
             */
            continue;
        }

        JsonEncodeString(key, out);
        fputc(':', out);
        CanonicalJsonEncodeValue(value, out);

        if (i < keyCount - 1)
        {
            fputc(',', out);
        }
    }

    fputc('}', out);

    ArrayFree(keys);
    return 1;
}
