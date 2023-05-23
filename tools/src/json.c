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
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <Args.h>
#include <Array.h>
#include <HashMap.h>
#include <Str.h>
#include <Memory.h>
#include <Json.h>

#define FLAG_ENCODE (1 << 0)
#define FLAG_SELECT (1 << 1)

static void
usage(char *prog)
{
    StreamPrintf(StreamStderr(), "Usage: %s [-s query|-e str]\n", prog);
}

static void
query(char *select, HashMap * json)
{
    char *key;
    JsonValue *val = JsonValueObject(json);

    key = strtok(select, "->");

    while (key)
    {
        char keyName[128];
        size_t arrInd;
        int expectArr = 0;
        int func = 0;

        expectArr = (sscanf(key, "%127[^[][%lu]", keyName, &arrInd) == 2);

        if (keyName[0] == '@')
        {
            if (StrEquals(keyName + 1, "length"))
            {
                switch (JsonValueType(val))
                {
                    case JSON_ARRAY:
                        val = JsonValueInteger(ArraySize(JsonValueAsArray(val)));
                        break;
                    case JSON_STRING:
                        val = JsonValueInteger(strlen(JsonValueAsString(val)));
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

                if (sscanf(keyName + 1, "%lu", &i) == 1)
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
        JsonEncodeValue(val, StreamStdout(), JSON_PRETTY);
        StreamPutc(StreamStdout(), '\n');
    }
}

static void
encode(char *str)
{
    JsonValue *val = JsonValueString(str);

    JsonEncodeValue(val, StreamStdout(), JSON_DEFAULT);
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

    ArgParseStateInit(&arg);
    while ((ch = ArgParse(&arg, args, "s:e:")) != -1)
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
            query(input, json);
            break;
        case FLAG_ENCODE:
            encode(input);
            break;
        default:
            JsonEncode(json, StreamStdout(), JSON_PRETTY);
            StreamPutc(StreamStdout(), '\n');
            break;
    }

    JsonFree(json);
    return 0;
}
