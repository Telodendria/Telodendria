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
#include <Util.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    FILE *stream;

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
    value->as.string = string;

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

void
JsonEncodeString(const char *str, FILE * out)
{
    size_t i;
    char c;

    fputc('"', out);

    i = 0;
    while ((c = str[i]) != '\0')
    {
        switch (c)
        {
            case '\\':
            case '"':
            case '/':
                fputc('\\', out);
                fputc(c, out);
                break;
            case '\b':
                fputs("\\b", out);
                break;
            case '\t':
                fputs("\\t", out);
                break;
            case '\n':
                fputs("\\n", out);
                break;
            case '\f':
                fputs("\\f", out);
                break;
            case '\r':
                fputs("\\r", out);
                break;
            default:              /* Assume UTF-8 input */
                /*
                 * RFC 4627: "All Unicode characters may be placed
                 * within the quotation marks except for the characters
                 * that must be escaped: quotation mark, reverse solidus,
                 * and the control characters (U+0000 through U+001F)."
                 *
                 * This technically covers the above cases for backspace,
                 * tab, newline, feed, and carriage return characters,
                 * but we can save bytes if we encode those as their
                 * more human-readable representation.
                 */
                if (c <= 0x001F)
                {
                    fprintf(out, "\\u%04x", c);
                }
                else
                {
                    fputc(c, out);
                }
                break;

        }
        i++;
    }

    fputc('"', out);
}

static char *
JsonDecodeString(FILE * in)
{
    const size_t strBlockSize = 16;

    size_t i;
    size_t len;
    size_t allocated;
    char *str;
    int c;
    char a[5];

    unsigned long utf8;
    char *utf8Ptr;

    len = 0;
    allocated = strBlockSize;

    str = Malloc(allocated * sizeof(char));
    if (!str)
    {
        return NULL;
    }

    while ((c = fgetc(in)) != EOF)
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
                str[len] = '\0';
                return str;
                break;
            case '\\':
                c = fgetc(in);
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
                        /* Read \uXXXX point into a 4-byte buffer */
                        if (fscanf(in, "%04lx", &utf8) != 1)
                        {
                            /* Bad hex value */
                            Free(str);
                            return NULL;
                        }

                        if (utf8 == 0)
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
                        utf8Ptr = UtilUtf8Encode(utf8);
                        if (!utf8Ptr)
                        {
                            /* Mem error */
                            Free(str);
                            return NULL;
                        }

                        /* Move the output of UtilUtf8Encode() into our
                         * local buffer */
                        strcpy(a, utf8Ptr);
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

void
JsonEncodeValue(JsonValue * value, FILE * out)
{
    size_t i;
    size_t len;
    Array *arr;

    switch (value->type)
    {
        case JSON_OBJECT:
            JsonEncode(value->as.object, out);
            break;
        case JSON_ARRAY:
            arr = value->as.array;
            len = ArraySize(arr);

            fputc('[', out);

            for (i = 0; i < len; i++)
            {
                JsonEncodeValue(ArrayGet(arr, i), out);
                if (i < len - 1)
                {
                    fputc(',', out);
                }
            }

            fputc(']', out);
            break;
        case JSON_STRING:
            JsonEncodeString(value->as.string, out);
            break;
        case JSON_INTEGER:
            fprintf(out, "%ld", value->as.integer);
            break;
        case JSON_FLOAT:
            fprintf(out, "%f", value->as.floating);
            break;
        case JSON_BOOLEAN:
            if (value->as.boolean)
            {
                fputs("true", out);
            }
            else
            {
                fputs("false", out);
            }
            break;
        case JSON_NULL:
            fputs("null", out);
            break;
        default:
            return;
    }
}

int
JsonEncode(HashMap * object, FILE * out)
{
    size_t index;
    size_t count;
    char *key;
    JsonValue *value;

    if (!object || !out)
    {
        return 0;
    }

    count = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        count++;
    }

    fputc('{', out);

    index = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        JsonEncodeString(key, out);
        fputc(':', out);
        JsonEncodeValue(value, out);

        if (index < count - 1)
        {
            fputc(',', out);
        }

        index++;
    }

    fputc('}', out);

    return 1;
}

void
JsonFree(HashMap * object)
{
    char *key;
    JsonValue *value;

    while (HashMapIterate(object, &key, (void **) &value))
    {
        /*
         * The key might not always be on the heap. In cases
         * where the JSON object is built programmatically instead
         * of with the parser, stack strings will probably have been
         * used as the key.
         */
        MemoryInfo *i = MemoryInfoGet(key);

        if (i)
        {
            Free(key);
        }

        JsonValueFree(value);
    }

    HashMapFree(object);
}

static int
JsonConsumeWhitespace(JsonParserState * state)
{
    int c;

    while (isspace(c = fgetc(state->stream)));

    return c;
}

static void
JsonTokenSeek(JsonParserState * state)
{
    int c = JsonConsumeWhitespace(state);

    if (feof(state->stream))
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

                while ((c = fgetc(state->stream)) != EOF)
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
                        ungetc(c, state->stream);
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
                        if (!fgets(state->token + 1, 4, state->stream))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (!strcmp("true", state->token))
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
                        if (!fgets(state->token + 1, 5, state->stream))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (!strcmp("false", state->token))
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
                        if (!fgets(state->token + 1, 4, state->stream))
                        {
                            state->tokenType = TOKEN_EOF;
                            Free(state->token);
                            state->token = NULL;
                            return;
                        }

                        if (!strcmp("null", state->token))
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
            strValue = Malloc(state->tokenLen);
            if (!strValue)
            {
                return NULL;
            }
            strcpy(strValue, state->token);
            value = JsonValueString(strValue);
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
            strcpy(key, state->token);

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

            JsonTokenSeek(state);
            if (JsonExpect(state, TOKEN_OBJECT_CLOSE))
            {
                break;
            }

            if (JsonExpect(state, TOKEN_COMMA))
            {
                continue;
            }

            goto error;
        }
        else if (JsonExpect(state, TOKEN_OBJECT_CLOSE))
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

    if (!arr)
    {
        return NULL;
    }

    do
    {
        JsonValue *value;

        JsonTokenSeek(state);
        value = JsonDecodeValue(state);

        if (!value)
        {
            goto error;
        }

        ArrayAdd(arr, value);

        JsonTokenSeek(state);

        if (JsonExpect(state, TOKEN_ARRAY_CLOSE))
        {
            break;
        }

        if (JsonExpect(state, TOKEN_COMMA))
        {
            continue;
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
JsonDecode(FILE * stream)
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
