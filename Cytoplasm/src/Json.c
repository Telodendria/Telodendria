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
#include <Json.h>

#include <Memory.h>
#include <Str.h>
#include <Util.h>
#include <Int.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

struct JsonValue
{
    JsonType type;
    union
    {
        HashMap *object;
        Array *array;
        char *string;
        long integer;
        double floating;
        int boolean:1;
    } as;
};

typedef enum JsonToken
{
    TOKEN_UNKNOWN,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_OBJECT_OPEN,
    TOKEN_OBJECT_CLOSE,
    TOKEN_ARRAY_OPEN,
    TOKEN_ARRAY_CLOSE,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_BOOLEAN,
    TOKEN_NULL,
    TOKEN_EOF
} JsonToken;

typedef struct JsonParserState
{
    Stream *stream;

    JsonToken tokenType;
    char *token;
    size_t tokenLen;
} JsonParserState;

JsonType
JsonValueType(JsonValue * value)
{
    if (!value)
    {
        return JSON_NULL;
    }

    return value->type;
}

static JsonValue *
JsonValueAllocate(void)
{
    return Malloc(sizeof(JsonValue));
}

JsonValue *
JsonValueObject(HashMap * object)
{
    JsonValue *value;

    if (!object)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_OBJECT;
    value->as.object = object;

    return value;
}

HashMap *
JsonValueAsObject(JsonValue * value)
{
    if (!value || value->type != JSON_OBJECT)
    {
        return NULL;
    }

    return value->as.object;
}


JsonValue *
JsonValueArray(Array * array)
{
    JsonValue *value;

    if (!array)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_ARRAY;
    value->as.array = array;

    return value;
}

Array *
JsonValueAsArray(JsonValue * value)
{
    if (!value || value->type != JSON_ARRAY)
    {
        return NULL;
    }

    return value->as.array;
}


JsonValue *
JsonValueString(char *string)
{
    JsonValue *value;

    if (!string)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_STRING;

    value->as.string = StrDuplicate(string);
    if (!value->as.string)
    {
        Free(value);
        return NULL;
    }

    return value;
}

char *
JsonValueAsString(JsonValue * value)
{
    if (!value || value->type != JSON_STRING)
    {
        return NULL;
    }

    return value->as.string;
}

JsonValue *
JsonValueInteger(long integer)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return 0;
    }

    value->type = JSON_INTEGER;
    value->as.integer = integer;

    return value;
}

long
JsonValueAsInteger(JsonValue * value)
{
    if (!value || value->type != JSON_INTEGER)
    {
        return 0;
    }

    return value->as.integer;
}


JsonValue *
JsonValueFloat(double floating)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_FLOAT;
    value->as.floating = floating;

    return value;
}

double
JsonValueAsFloat(JsonValue * value)
{
    if (!value || value->type != JSON_FLOAT)
    {
        return 0;
    }

    return value->as.floating;
}

JsonValue *
JsonValueBoolean(int boolean)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOLEAN;
    value->as.boolean = boolean;

    return value;
}

int
JsonValueAsBoolean(JsonValue * value)
{
    if (!value || value->type != JSON_BOOLEAN)
    {
        return 0;
    }

    return value->as.boolean;
}

JsonValue *
JsonValueNull(void)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NULL;

    return value;
}

void
JsonValueFree(JsonValue * value)
{
    size_t i;
    Array *arr;

    if (!value)
    {
        return;
    }

    switch (value->type)
    {
        case JSON_OBJECT:
            JsonFree(value->as.object);
            break;
        case JSON_ARRAY:
            arr = value->as.array;
            for (i = 0; i < ArraySize(arr); i++)
            {
                JsonValueFree((JsonValue *) ArrayGet(arr, i));
            }
            ArrayFree(arr);
            break;
        case JSON_STRING:
            Free(value->as.string);
            break;
        default:
            break;
    }

    Free(value);
}

int
JsonEncodeString(const char *str, Stream * out)
{
    size_t i;
    char c;
    int length = 0;

    StreamPutc(out, '"');
    length++;

    i = 0;
    while ((c = str[i]) != '\0')
    {
        switch (c)
        {
            case '\\':
            case '"':
            case '/':
                StreamPutc(out, '\\');
                StreamPutc(out, c);
                length += 2;
                break;
            case '\b':
                StreamPuts(out, "\\b");
                length += 2;
                break;
            case '\t':
                StreamPuts(out, "\\t");
                length += 2;
                break;
            case '\n':
                StreamPuts(out, "\\n");
                length += 2;
                break;
            case '\f':
                StreamPuts(out, "\\f");
                length += 2;
                break;
            case '\r':
                StreamPuts(out, "\\r");
                length += 2;
                break;
            default:              /* Assume UTF-8 input */
                StreamPutc(out, c);
                length++;
                break;
        }
        i++;
    }

    StreamPutc(out, '"');
    length++;

    return length;
}

static char *
JsonDecodeString(Stream * in)
{
    const size_t strBlockSize = 16;

    size_t i;
    size_t len;
    size_t allocated;
    char *str;
    int c;
    char a[5];

    UInt32 codepoint;
    UInt16 high;
    UInt16 low;

    char *utf8Ptr;

    len = 0;
    allocated = strBlockSize;

    str = Malloc(allocated * sizeof(char));
    if (!str)
    {
        return NULL;
    }

    while ((c = StreamGetc(in)) != EOF)
    {
        if (c <= 0x001F)
        {
            /* Bad byte; these must  be escaped */
            Free(str);
            return NULL;
        }

        switch (c)
        {
            case '"':
                if (len >= allocated)
                {
                    char *tmp;

                    allocated += 1;
                    tmp = Realloc(str, allocated * sizeof(char));
                    if (!tmp)
                    {
                        Free(str);
                        return NULL;
                    }

                    str = tmp;
                }
                str[len] = '\0';
                return str;
                break;
            case '\\':
                c = StreamGetc(in);
                switch (c)
                {
                    case '\\':
                    case '"':
                    case '/':
                        a[0] = c;
                        a[1] = '\0';
                        break;
                    case 'b':
                        a[0] = '\b';
                        a[1] = '\0';
                        break;
                    case 't':
                        a[0] = '\t';
                        a[1] = '\0';
                        break;
                    case 'n':
                        a[0] = '\n';
                        a[1] = '\0';
                        break;
                    case 'f':
                        a[0] = '\f';
                        a[1] = '\0';
                        break;
                    case 'r':
                        a[0] = '\r';
                        a[1] = '\0';
                        break;
                    case 'u':
                        /* Read 4 characters into a */
                        if (!StreamGets(in, a, sizeof(a)))
                        {
                            Free(str);
                            return NULL;
                        }
                        /* Interpret characters as a hex number */
                        if (sscanf(a, "%04hx", &high) != 1)
                        {
                            /* Bad hex value */
                            Free(str);
                            return NULL;
                        }

                        /* If this is a two-byte UTF-16 codepoint, grab
                         * the second byte */
                        if (high > 0xD7FF && high <= 0xDBFF)
                        {
                            if (StreamGetc(in) != '\\' || StreamGetc(in) != 'u')
                            {
                                Free(str);
                                return NULL;
                            }

                            /* Read 4 characters into a */
                            if (!StreamGets(in, a, sizeof(a)))
                            {
                                Free(str);
                                return NULL;
                            }

                            /* Interpret characters as a hex number */
                            if (sscanf(a, "%04hx", &low) != 1)
                            {
                                Free(str);
                                return NULL;
                            }

                            codepoint = StrUtf16Decode(high, low);
                        }
                        else
                        {
                            codepoint = high;
                        }


                        if (codepoint == 0)
                        {
                            /*
                             * We read in a 0000, null. There is no
                             * circumstance in which putting a null
                             * character into our buffer will end well.
                             *
                             * There's also really no legitimate use
                             * for the null character in our JSON anyway;
                             * it's likely an attempted exploit.
                             *
                             * So lets just strip it out. Don't even
                             * include it in the string. There should be
                             * no harm in ignoring it.
                             */
                            continue;
                        }

                        /* Encode the 4-byte UTF-8 buffer into a series
                         * of 1-byte characters */
                        utf8Ptr = StrUtf8Encode(codepoint);
                        if (!utf8Ptr)
                        {
                            /* Mem error */
                            Free(str);
                            return NULL;
                        }

                        /* Move the output of StrUtf8Encode() into our
                         * local buffer */
                        strncpy(a, utf8Ptr, sizeof(a) - 1);
                        Free(utf8Ptr);
                        break;
                    default:
                        /* Bad escape value */
                        Free(str);
                        return NULL;
                }
                break;
            default:
                a[0] = c;
                a[1] = '\0';
                break;
        }

        /* Append buffer a */
        i = 0;
        while (a[i] != '\0')
        {
            if (len >= allocated)
            {
                char *tmp;

                allocated += strBlockSize;
                tmp = Realloc(str, allocated * sizeof(char));
                if (!tmp)
                {
                    Free(str);
                    return NULL;
                }

                str = tmp;
            }

            str[len] = a[i];
            len++;
            i++;
        }
    }

    Free(str);
    return NULL;
}

int
JsonEncodeValue(JsonValue * value, Stream * out, int level)
{
    size_t i;
    size_t len;
    Array *arr;
    int length = 0;

    switch (value->type)
    {
        case JSON_OBJECT:
            length += JsonEncode(value->as.object, out, level >= 0 ? level : level);
            break;
        case JSON_ARRAY:
            arr = value->as.array;
            len = ArraySize(arr);

            StreamPutc(out, '[');
            length++;
            for (i = 0; i < len; i++)
            {
                if (level >= 0)
                {
                    length += StreamPrintf(out, "\n%*s", level + 2, "");
                }
                length += JsonEncodeValue(ArrayGet(arr, i), out, level >= 0 ? level + 2 : level);
                if (i < len - 1)
                {
                    StreamPutc(out, ',');
                    length++;
                }
            }

            if (level >= 0)
            {
                length += StreamPrintf(out, "\n%*s", level, "");
            }
            StreamPutc(out, ']');
            length++;
            break;
        case JSON_STRING:
            length += JsonEncodeString(value->as.string, out);
            break;
        case JSON_INTEGER:
            length += StreamPrintf(out, "%ld", value->as.integer);
            break;
        case JSON_FLOAT:
            length += StreamPrintf(out, "%f", value->as.floating);
            break;
        case JSON_BOOLEAN:
            if (value->as.boolean)
            {
                StreamPuts(out, "true");
                length += 4;
            }
            else
            {
                StreamPuts(out, "false");
                length += 5;
            }
            break;
        case JSON_NULL:
            StreamPuts(out, "null");
            length += 4;
            break;
        default:
            return -1;
    }

    return length;
}

int
JsonEncode(HashMap * object, Stream * out, int level)
{
    size_t index;
    size_t count;
    char *key;
    JsonValue *value;
    int length;

    if (!object)
    {
        return -1;
    }

    count = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        count++;
    }

    /* The total number of bytes written */
    length = 0;

    StreamPutc(out, '{');
    length++;

    if (level >= 0)
    {
        StreamPutc(out, '\n');
        length++;
    }

    index = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        if (level >= 0)
        {
            StreamPrintf(out, "%*s", level + 2, "");
            length += level + 2;
        }

        length += JsonEncodeString(key, out);

        StreamPutc(out, ':');
        length++;
        if (level >= 0)
        {
            StreamPutc(out, ' ');
            length++;
        }

        length += JsonEncodeValue(value, out, level >= 0 ? level + 2 : level);

        if (index < count - 1)
        {
            StreamPutc(out, ',');
            length++;
        }

        if (level >= 0)
        {
            StreamPutc(out, '\n');
            length++;
        }

        index++;
    }

    if (level >= 0)
    {
        StreamPrintf(out, "%*s", level, "");
        length += level;
    }
    StreamPutc(out, '}');
    length++;

    return length;
}

void
JsonFree(HashMap * object)
{
    char *key;
    JsonValue *value;

    if (!object)
    {
        return;
    }

    while (HashMapIterate(object, &key, (void **) &value))
    {
        JsonValueFree(value);
    }

    HashMapFree(object);
}

JsonValue *
JsonValueDuplicate(JsonValue * val)
{
    JsonValue *new;
    size_t i;

    if (!val)
    {
        return NULL;
    }

    new = JsonValueAllocate();
    if (!new)
    {
        return NULL;
    }

    new->type = val->type;

    switch (val->type)
    {
        case JSON_OBJECT:
            new->as.object = JsonDuplicate(val->as.object);
            break;
        case JSON_ARRAY:
            new->as.array = ArrayCreate();
            for (i = 0; i < ArraySize(val->as.array); i++)
            {
                ArrayAdd(new->as.array, JsonValueDuplicate(ArrayGet(val->as.array, i)));
            }
            break;
        case JSON_STRING:
            new->as.string = StrDuplicate(val->as.string);
            break;
        case JSON_INTEGER:
        case JSON_FLOAT:
        case JSON_BOOLEAN:
            /* These are by value, not by reference */
            new->as = val->as;
        case JSON_NULL:
        default:
            break;
    }

    return new;
}

HashMap *
JsonDuplicate(HashMap * object)
{
    HashMap *new;
    char *key;
    JsonValue *val;

    if (!object)
    {
        return NULL;
    }

    new = HashMapCreate();
    if (!new)
    {
        return NULL;
    }

    while (HashMapIterate(object, &key, (void **) &val))
    {
        HashMapSet(new, key, JsonValueDuplicate(val));
    }

    return new;
}

static int
JsonConsumeWhitespace(JsonParserState * state)
{
    static const int nRetries = 5;
    static const int delay = 2;

    int c;
    int tries = 0;
    int readFlg = 0;

    while (1)
    {
        c = StreamGetc(state->stream);

        if (StreamEof(state->stream))
        {
            break;
        }

        if (StreamError(state->stream))
        {
            if (errno == EAGAIN)
            {
                StreamClearError(state->stream);
                tries++;

                if (tries >= nRetries || readFlg)
                {
                    break;
                }
                else
                {
                    UtilSleepMillis(UInt64Create(0, delay));
                    continue;
                }
            }
            else
            {
                break;
            }
        }

        /* As soon as we've successfully read a byte, treat future
         * EAGAINs as EOF, because some clients don't properly shutdown
         * their sockets. */
        readFlg = 1;
        tries = 0;

        if (!isspace(c))
        {
            break;
        }
    }

    return c;
}

static void
JsonTokenSeek(JsonParserState * state)
{
    int c = JsonConsumeWhitespace(state);

    if (StreamEof(state->stream))
    {
        state->tokenType = TOKEN_EOF;
        return;
    }

    if (state->token)
    {
        Free(state->token);
        state->token = NULL;
    }

    switch (c)
    {
        case ':':
            state->tokenType = TOKEN_COLON;
            break;
        case ',':
            state->tokenType = TOKEN_COMMA;
            break;
        case '{':
            state->tokenType = TOKEN_OBJECT_OPEN;
            break;
        case '}':
            state->tokenType = TOKEN_OBJECT_CLOSE;
            break;
        case '[':
            state->tokenType = TOKEN_ARRAY_OPEN;
            break;
        case ']':
            state->tokenType = TOKEN_ARRAY_CLOSE;
            break;
        case '"':
            state->token = JsonDecodeString(state->stream);
            if (!state->token)
            {
                state->tokenType = TOKEN_EOF;
                return;
            }
            state->tokenType = TOKEN_STRING;
            state->tokenLen = strlen(state->token);
            break;
        default:
            if (c == '-' || isdigit(c))
            {
                int isFloat = 0;
                size_t allocated = 16;

                state->tokenLen = 1;
                state->token = Malloc(allocated);
                if (!state->token)
                {
                    state->tokenType = TOKEN_EOF;
                    return;
                }
                state->token[0] = c;

                while ((c = StreamGetc(state->stream)) != EOF)
                {
                    if (c == '.')
                    {
                        if (state->tokenLen > 1 && !isFloat)
                        {
                            isFloat = 1;
                        }
                        else
                        {
                            state->tokenType = TOKEN_UNKNOWN;
                            return;
                        }
                    }
                    else if (!isdigit(c))
                    {
                        StreamUngetc(state->stream, c);
                        break;
                    }

                    if (state->tokenLen >= allocated)
                    {
                        char *tmp;

                        allocated += 16;

                        tmp = Realloc(state->token, allocated);
                        if (!tmp)
                        {
                            state->tokenType = TOKEN_EOF;
                            return;
                        }

                        state->token = tmp;
                    }

                    state->token[state->tokenLen] = c;
                    state->tokenLen++;
                }

                if (state->token[state->tokenLen - 1] == '.')
                {
                    state->tokenType = TOKEN_UNKNOWN;
                    return;
                }

                state->token[state->tokenLen] = '\0';
                if (isFloat)
                {
                    state->tokenType = TOKEN_FLOAT;
                }
                else
                {
                    state->tokenType = TOKEN_INTEGER;
                }
            }
            else
            {
                state->tokenLen = 8;
                state->token = Malloc(state->tokenLen);
                if (!state->token)
                {
                    state->tokenType = TOKEN_EOF;
                    return;
                }

                state->token[0] = c;

                switch (c)
                {
                    case 't':
                        if (!StreamGets(state->stream, state->token + 1, 4))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (StrEquals("true", state->token))
                        {
                            state->tokenType = TOKEN_BOOLEAN;
                            state->tokenLen = 5;
                        }
                        else
                        {
                            state->tokenType = TOKEN_UNKNOWN;
                            Free(state->token);
                            state->token = NULL;
                        }
                        break;
                    case 'f':
                        if (!StreamGets(state->stream, state->token + 1, 5))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (StrEquals("false", state->token))
                        {
                            state->tokenType = TOKEN_BOOLEAN;
                            state->tokenLen = 6;
                        }
                        else
                        {
                            state->tokenType = TOKEN_UNKNOWN;
                            Free(state->token);
                            state->token = NULL;
                        }
                        break;
                    case 'n':
                        if (!StreamGets(state->stream, state->token + 1, 4))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (StrEquals("null", state->token))
                        {
                            state->tokenType = TOKEN_NULL;
                        }
                        else
                        {
                            state->tokenType = TOKEN_UNKNOWN;
                            Free(state->token);
                            state->token = NULL;
                        }
                        break;
                    default:
                        state->tokenType = TOKEN_UNKNOWN;
                        Free(state->token);
                        state->token = NULL;
                        break;
                }
            }
    }
}

static int
JsonExpect(JsonParserState * state, JsonToken token)
{
    return state->tokenType == token;
}

static Array *
 JsonDecodeArray(JsonParserState *);

static HashMap *
 JsonDecodeObject(JsonParserState *);

static JsonValue *
JsonDecodeValue(JsonParserState * state)
{
    JsonValue *value;
    char *strValue;

    switch (state->tokenType)
    {
        case TOKEN_OBJECT_OPEN:
            value = JsonValueObject(JsonDecodeObject(state));
            break;
        case TOKEN_ARRAY_OPEN:
            value = JsonValueArray(JsonDecodeArray(state));
            break;
        case TOKEN_STRING:
            strValue = Malloc(state->tokenLen + 1);
            if (!strValue)
            {
                return NULL;
            }
            strncpy(strValue, state->token, state->tokenLen + 1);
            value = JsonValueString(strValue);
            Free(strValue);
            break;
        case TOKEN_INTEGER:
            value = JsonValueInteger(atol(state->token));
            break;
        case TOKEN_FLOAT:
            value = JsonValueFloat(atof(state->token));
            break;
        case TOKEN_BOOLEAN:
            value = JsonValueBoolean(state->token[0] == 't');
            break;
        case TOKEN_NULL:
            value = JsonValueNull();
            break;
        default:
            value = NULL;
            break;
    }

    return value;
}

static HashMap *
JsonDecodeObject(JsonParserState * state)
{
    HashMap *obj = HashMapCreate();
    int comma = 0;

    if (!obj)
    {
        return NULL;
    }

    do
    {
        JsonTokenSeek(state);
        if (JsonExpect(state, TOKEN_STRING))
        {
            char *key = Malloc(state->tokenLen + 1);
            JsonValue *value;

            if (!key)
            {
                goto error;
            }
            strncpy(key, state->token, state->tokenLen + 1);

            JsonTokenSeek(state);
            if (!JsonExpect(state, TOKEN_COLON))
            {
                Free(key);
                goto error;
            }

            JsonTokenSeek(state);
            value = JsonDecodeValue(state);

            if (!value)
            {
                Free(key);
                goto error;
            }

            /* If there's an existing value at this key, discard it. */
            JsonValueFree(HashMapSet(obj, key, value));
            Free(key);

            JsonTokenSeek(state);

            if (JsonExpect(state, TOKEN_COMMA))
            {
                comma = 1;
                continue;
            }

            if (JsonExpect(state, TOKEN_OBJECT_CLOSE))
            {
                break;
            }

            goto error;
        }
        else if (!comma && JsonExpect(state, TOKEN_OBJECT_CLOSE))
        {
            break;
        }
        else
        {
            goto error;
        }
    } while (!JsonExpect(state, TOKEN_EOF));

    return obj;
error:
    JsonFree(obj);
    return NULL;
}

static Array *
JsonDecodeArray(JsonParserState * state)
{
    Array *arr = ArrayCreate();
    size_t i;
    int comma = 0;

    if (!arr)
    {
        return NULL;
    }

    do
    {
        JsonValue *value;

        JsonTokenSeek(state);

        if (!comma && JsonExpect(state, TOKEN_ARRAY_CLOSE))
        {
            break;
        }

        value = JsonDecodeValue(state);

        if (!value)
        {
            goto error;
        }

        ArrayAdd(arr, value);

        JsonTokenSeek(state);

        if (JsonExpect(state, TOKEN_COMMA))
        {
            comma = 1;
            continue;
        }

        if (JsonExpect(state, TOKEN_ARRAY_CLOSE))
        {
            break;
        }

        goto error;
    } while (!JsonExpect(state, TOKEN_EOF));

    return arr;
error:
    for (i = 0; i < ArraySize(arr); i++)
    {
        JsonValueFree((JsonValue *) ArrayGet(arr, i));
    }
    ArrayFree(arr);
    return NULL;
}

HashMap *
JsonDecode(Stream * stream)
{
    HashMap *result;

    JsonParserState state;

    state.stream = stream;
    state.token = NULL;

    JsonTokenSeek(&state);
    if (!JsonExpect(&state, TOKEN_OBJECT_OPEN))
    {
        return NULL;
    }

    result = JsonDecodeObject(&state);

    if (state.token)
    {
        Free(state.token);
    }

    return result;
}

JsonValue *
JsonGet(HashMap * json, size_t nArgs,...)
{
    va_list argp;

    HashMap *tmp = json;
    JsonValue *val = NULL;
    size_t i;

    if (!json || !nArgs)
    {
        return NULL;
    }

    va_start(argp, nArgs);
    for (i = 0; i < nArgs - 1; i++)
    {
        char *key = va_arg(argp, char *);

        val = HashMapGet(tmp, key);
        if (!val)
        {
            goto finish;
        }

        if (JsonValueType(val) != JSON_OBJECT)
        {
            val = NULL;
            goto finish;
        }

        tmp = JsonValueAsObject(val);
    }

    val = HashMapGet(tmp, va_arg(argp, char *));

finish:
    va_end(argp);
    return val;
}

JsonValue *
JsonSet(HashMap * json, JsonValue * newVal, size_t nArgs,...)
{
    HashMap *tmp = json;
    JsonValue *val = NULL;
    size_t i;

    va_list argp;

    if (!json || !newVal || !nArgs)
    {
        return NULL;
    }

    va_start(argp, nArgs);

    for (i = 0; i < nArgs - 1; i++)
    {
        char *key = va_arg(argp, char *);

        val = HashMapGet(tmp, key);
        if (!val)
        {
            val = JsonValueObject(HashMapCreate());
            HashMapSet(tmp, key, val);
        }

        if (JsonValueType(val) != JSON_OBJECT)
        {
            val = NULL;
            goto finish;
        }

        tmp = JsonValueAsObject(val);
    }

    val = HashMapSet(tmp, va_arg(argp, char *), newVal);

finish:
    va_end(argp);
    return val;
}
