/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
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
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <Cytoplasm/Args.h>
#include <Cytoplasm/Array.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Memory.h>

#include <Cytoplasm/Json.h>
#include <CanonicalJson.h>

#define FLAG_ENCODE (1 << 0)
#define FLAG_SELECT (1 << 1)

static void
usage(char *prog)
{
    StreamPrintf(StreamStderr(), "Usage: %s [-s query|-e str]\n", prog);
}

static void
query(char *select, HashMap * json, int canonical)
{
    char *key;
    JsonValue *rootVal = JsonValueObject(json);
    JsonValue *val = rootVal;
    Array *cleanUp = ArrayCreate();
    size_t i;

    ArrayAdd(cleanUp, rootVal);

    key = strtok(select, "->");

    while (key)
    {
        char keyName[128];
        size_t arrInd;
        int expectArr = 0;
        int func = 0;

        expectArr = (sscanf(key, "%127[^[][%zu]", keyName, &arrInd) == 2);

        if (keyName[0] == '@')
        {
            if (StrEquals(keyName + 1, "length"))
            {
                uint64_t len;

                switch (JsonValueType(val))
                {
                    case JSON_ARRAY:
                        len = ArraySize(JsonValueAsArray(val));
                        val = JsonValueInteger(len);
                        ArrayAdd(cleanUp, val);
                        break;
                    case JSON_STRING:
                        len = strlen(JsonValueAsString(val));
                        val = JsonValueInteger(len);
                        ArrayAdd(cleanUp, val);
                        break;
                    default:
                        val = NULL;
                        break;
                }
            }
            else if (JsonValueType(val) == JSON_OBJECT && StrEquals(keyName + 1, "keys"))
            {
                HashMap *obj = JsonValueAsObject(val);
                Array *arr = ArrayCreate();
                char *k;
                void *v;

                while (HashMapIterate(obj, &k, &v))
                {
                    ArrayAdd(arr, JsonValueString(k));
                }

                val = JsonValueArray(arr);
                ArrayAdd(cleanUp, val);
            }
            else if (JsonValueType(val) == JSON_STRING && StrEquals(keyName + 1, "decode"))
            {
                StreamPrintf(StreamStdout(), "%s\n", JsonValueAsString(val));
                val = NULL;
                break;
            }
            else
            {
                val = NULL;
                break;
            }

            func = 1;
        }
        else if (keyName[0] == '^')
        {
            if (JsonValueType(val) == JSON_OBJECT)
            {
                HashMap *obj = JsonValueAsObject(val);

                JsonValueFree(HashMapDelete(obj, keyName + 1));
            }
            else if (JsonValueType(val) == JSON_ARRAY)
            {
                size_t i;

                if (sscanf(keyName + 1, "%zu", &i) == 1)
                {
                    JsonValueFree(ArrayDelete(JsonValueAsArray(val), i));
                }
                else
                {
                    val = NULL;
                    break;
                }
            }
            else
            {
                val = NULL;
                break;
            }

            func = 1;
        }

        if (!func)
        {
            if (JsonValueType(val) == JSON_OBJECT)
            {
                val = HashMapGet(JsonValueAsObject(val), keyName);
            }
            else
            {
                val = NULL;
                break;
            }
        }

        if (expectArr && JsonValueType(val) == JSON_ARRAY)
        {
            val = ArrayGet(JsonValueAsArray(val), arrInd);
        }

        if (!val)
        {
            break;
        }

        key = strtok(NULL, "->");
    }

    if (val)
    {
        if (canonical)
        {
            CanonicalJsonEncodeValue(val, StreamStdout());
        }
        else
        {
            JsonEncodeValue(val, StreamStdout(), JSON_PRETTY);
        }

        StreamPutc(StreamStdout(), '\n');
    }

    for (i = 0; i < ArraySize(cleanUp); i++)
    {
        JsonValueFree(ArrayGet(cleanUp, i));
    }
    ArrayFree(cleanUp);
}

static void
encode(char *str, int canonical)
{
    JsonValue *val = JsonValueString(str);

    if (canonical)
    {
        CanonicalJsonEncodeValue(val, StreamStdout());
    }
    else
    {
        JsonEncodeValue(val, StreamStdout(), JSON_DEFAULT);
    }

    JsonValueFree(val);
    StreamPutc(StreamStdout(), '\n');
}

int
Main(Array * args)
{
    HashMap *json = NULL;
    int flag = 0;
    int ch;
    char *input = NULL;
    ArgParseState arg;

    int canonical = 0;

    ArgParseStateInit(&arg);
    while ((ch = ArgParse(&arg, args, "cs:e:")) != -1)
    {
        switch (ch)
        {
            case 's':
                flag = FLAG_SELECT;
                input = arg.optArg;
                break;
            case 'e':
                flag = FLAG_ENCODE;
                input = arg.optArg;
                break;
            case 'c':
                canonical = 1;
                break;
            default:
                usage(ArrayGet(args, 0));
                return 1;
        }
    }

    if (flag != FLAG_ENCODE)
    {
        json = JsonDecode(StreamStdin());

        if (!json)
        {
            StreamPuts(StreamStderr(), "Malformed JSON.\n");
            return 1;
        }
    }

    switch (flag)
    {
        case FLAG_SELECT:
            query(input, json, canonical);      /* This will implicitly
                                                 * free json */
            break;
        case FLAG_ENCODE:
            encode(input, canonical);
            break;
        default:
            if (canonical)
            {
                CanonicalJsonEncode(json, StreamStdout());
            }
            else
            {
                JsonEncode(json, StreamStdout(), JSON_PRETTY);
            }

            StreamPutc(StreamStdout(), '\n');
            JsonFree(json);
            break;
    }

    return 0;
}
