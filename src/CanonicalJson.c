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
#include <CanonicalJson.h>

#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Array.h>
#include <Cytoplasm/Json.h>

#include <stdio.h>
#include <string.h>

static int
CanonicalJsonKeyCompare(void *k1, void *k2)
{
    return strcmp((char *) k1, (char *) k2);
}

int
CanonicalJsonEncodeValue(JsonValue * value, Stream * out)
{
    Array *arr;
    size_t i, len;

    int length = 0;

    /* Override object type to encode using the canonical functions */
    switch (JsonValueType(value))
    {
        case JSON_OBJECT:
            length += CanonicalJsonEncode(JsonValueAsObject(value), out);
            break;
        case JSON_ARRAY:
            arr = JsonValueAsArray(value);
            len = ArraySize(arr);

            StreamPutc(out, '[');
            length++;

            for (i = 0; i < len; i++)
            {
                JsonValue *aVal = ArrayGet(arr, i);

                if (JsonValueType(aVal) == JSON_FLOAT)
                {
                    /* See comment in CanonicalJsonEncode() */
                    continue;
                }

                length += CanonicalJsonEncodeValue(aVal, out);
                if (i < len - 1)
                {
                    StreamPutc(out, ',');
                    length++;
                }
            }

            StreamPutc(out, ']');
            length++;
            break;
        default:
            length += JsonEncodeValue(value, out, JSON_DEFAULT);
            break;
    }

    return length;
}

int
CanonicalJsonEncode(HashMap * object, Stream * out)
{
    char *key;
    JsonValue *value;
    Array *keys;
    size_t i;
    size_t keyCount;
    int length;

    if (!object)
    {
        return -1;
    }

    keys = ArrayCreate();
    if (!keys)
    {
        return -1;
    }

    while (HashMapIterate(object, &key, (void **) &value))
    {
        ArrayAdd(keys, key);
    }

    ArraySort(keys, CanonicalJsonKeyCompare);

    /* The total number of bytes written */
    length = 0;

    StreamPutc(out, '{');
    length++;

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

        length += JsonEncodeString(key, out);
        StreamPutc(out, ':');
        length++;
        length += CanonicalJsonEncodeValue(value, out);

        if (i < keyCount - 1)
        {
            StreamPutc(out, ',');
            length++;
        }
    }

    StreamPutc(out, '}');
    length++;

    ArrayFree(keys);
    return length;
}
