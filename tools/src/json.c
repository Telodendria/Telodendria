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

#include <Array.h>
#include <HashMap.h>
#include <Memory.h>
#include <Json.h>

int
main(int argc, char **argv)
{
    HashMap *json;

    json = JsonDecode(stdin);

    if (!json)
    {
        fprintf(stderr, "Malformed JSON.\n");
        return 1;
    }

    if (argc > 1)
    {
        char *select = argv[1];
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
                if (strcmp(keyName + 1, "length") == 0)
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
                else if (JsonValueType(val) == JSON_OBJECT && strcmp(keyName + 1, "keys") == 0)
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
                else if (JsonValueType(val) == JSON_STRING && strcmp(keyName + 1, "decode") == 0)
                {
                    printf("%s", JsonValueAsString(val));
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
            else if (keyName[0] == '!')
            {
                if (JsonValueType(val) == JSON_OBJECT)
                {
                    HashMap *obj = JsonValueAsObject(val);

                    JsonValueFree(HashMapDelete(obj, keyName + 1));
                }
                else if (JsonValueType(val) == JSON_ARRAY)
                {
                    size_t i;

                    sscanf(keyName + 1, "%lu", &i);
                    JsonValueFree(ArrayDelete(JsonValueAsArray(val), i));
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
            JsonEncodeValue(val, stdout, JSON_PRETTY);
        }

        printf("\n");
    }
    else
    {
        JsonEncode(json, stdout, JSON_PRETTY);
        JsonFree(json);
    }

    MemoryFreeAll();
    return 0;
}
