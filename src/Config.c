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
#include <Config.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef CONFIG_BUFFER_BLOCK
#define CONFIG_BUFFER_BLOCK 32
#endif

struct ConfigDirective
{
    Array *values;
    HashMap *children;
};

struct ConfigParseResult
{
    unsigned int ok:1;
    union
    {
        size_t lineNumber;
        HashMap *confMap;
    } data;
};

typedef enum ConfigToken
{
    TOKEN_UNKNOWN,
    TOKEN_NAME,
    TOKEN_MACRO_ASSIGNMENT,
    TOKEN_VALUE,
    TOKEN_SEMICOLON,
    TOKEN_BLOCK_OPEN,
    TOKEN_BLOCK_CLOSE,
    TOKEN_MACRO,
    TOKEN_EOF
} ConfigToken;

typedef struct ConfigParserState
{
    FILE *stream;
    unsigned int line;

    char *token;
    size_t tokenSize;
    size_t tokenLen;
    ConfigToken tokenType;

    HashMap *macroMap;

} ConfigParserState;

unsigned int
ConfigParseResultOk(ConfigParseResult * result)
{
    return result ? result->ok : 0;
}

size_t
ConfigParseResultLineNumber(ConfigParseResult * result)
{
    return result && !result->ok ? result->data.lineNumber : 0;
}

HashMap *
ConfigParseResultGet(ConfigParseResult * result)
{
    return result && result->ok ? result->data.confMap : NULL;
}

void
ConfigParseResultFree(ConfigParseResult * result)
{
    /*
     * Note that if the parse was valid, the hash map
     * needs to be freed separately.
     */
    free(result);
}

Array *
ConfigValuesGet(ConfigDirective * directive)
{
    return directive ? directive->values : NULL;
}

HashMap *
ConfigChildrenGet(ConfigDirective * directive)
{
    return directive ? directive->children : NULL;
}

static void
ConfigDirectiveFree(ConfigDirective * directive)
{
    size_t i;

    if (!directive)
    {
        return;
    }

    for (i = 0; i < ArraySize(directive->values); i++)
    {
        free(ArrayGet(directive->values, i));
    }

    ArrayFree(directive->values);

    ConfigFree(directive->children);

    free(directive);
}

void
ConfigFree(HashMap * conf)
{
    char *key;
    void *value;

    while (HashMapIterate(conf, &key, &value))
    {
        ConfigDirectiveFree((ConfigDirective *) value);
        free(key);
    }

    HashMapFree(conf);
}

static ConfigParserState *
ConfigParserStateCreate(FILE * stream)
{
    ConfigParserState *state = malloc(sizeof(ConfigParserState));

    if (!state)
    {
        return NULL;
    }

    state->macroMap = HashMapCreate();

    if (!state->macroMap)
    {
        free(state);
        return NULL;
    }

    state->stream = stream;
    state->line = 1;
    state->token = NULL;
    state->tokenSize = 0;
    state->tokenLen = 0;
    state->tokenType = TOKEN_UNKNOWN;

    return state;
}

static void
ConfigParserStateFree(ConfigParserState * state)
{
    char *key;
    void *value;

    if (!state)
    {
        return;
    }


    free(state->token);

    while (HashMapIterate(state->macroMap, &key, &value))
    {
        free(key);
        free(value);
    }

    HashMapFree(state->macroMap);

    free(state);
}

static int
ConfigIsNameChar(int c)
{
    return isdigit(c) || isalpha(c) || (c == '-' || c == '_');
}

static char
ConfigConsumeWhitespace(ConfigParserState * state)
{
    int c;

    while (isspace(c = fgetc(state->stream)))
    {
        if (c == '\n')
        {
            state->line++;
        }
    }
    return c;
}

static void
ConfigConsumeLine(ConfigParserState * state)
{
    while (fgetc(state->stream) != '\n');
    state->line++;
}

static void
ConfigTokenSeek(ConfigParserState * state)
{
    int c;

    /* If we already hit EOF, don't do anything */
    if (state->tokenType == TOKEN_EOF)
    {
        return;
    }
    while ((c = ConfigConsumeWhitespace(state)) == '#')
    {
        ConfigConsumeLine(state);
    }

    /*
     * After all whitespace and comments are consumed, identify the
     * token by looking at the next character
     */

    if (feof(state->stream))
    {
        state->tokenType = TOKEN_EOF;
        return;
    }
    if (ConfigIsNameChar(c))
    {
        state->tokenLen = 0;

        /* Read the key/macro into state->token */
        if (!state->token)
        {
            state->tokenSize = CONFIG_BUFFER_BLOCK;
            state->token = malloc(CONFIG_BUFFER_BLOCK);
        }
        state->token[state->tokenLen] = c;
        state->tokenLen++;

        while (ConfigIsNameChar((c = fgetc(state->stream))))
        {
            state->token[state->tokenLen] = c;
            state->tokenLen++;

            if (state->tokenLen >= state->tokenSize)
            {
                state->tokenSize += CONFIG_BUFFER_BLOCK;
                state->token = realloc(state->token,
                                       state->tokenSize);
            }
        }

        state->token[state->tokenLen] = '\0';
        state->tokenLen++;

        if (!isspace(c))
        {
            state->tokenType = TOKEN_UNKNOWN;
        }
        else
        {
            state->tokenType = TOKEN_NAME;

            if (c == '\n')
            {
                state->line++;
            }
        }

    }
    else
    {
        switch (c)
        {
            case '=':
                state->tokenType = TOKEN_MACRO_ASSIGNMENT;
                break;
            case '"':
                state->tokenLen = 0;
                state->tokenType = TOKEN_VALUE;

                /* read the value into state->curtok */
                while ((c = fgetc(state->stream)) != '"')
                {
                    if (c == '\n')
                    {
                        state->line++;
                    }
                    /*
                     * End of the stream reached without finding
                     * a closing quote
                     */
                    if (feof(state->stream))
                    {
                        state->tokenType = TOKEN_EOF;
                        break;
                    }
                    state->token[state->tokenLen] = c;
                    state->tokenLen++;

                    if (state->tokenLen >= state->tokenSize)
                    {
                        state->tokenSize += CONFIG_BUFFER_BLOCK;
                        state->token = realloc(state->token,
                                               state->tokenSize);
                    }
                }
                state->token[state->tokenLen] = '\0';
                state->tokenLen++;
                break;
            case ';':
                state->tokenType = TOKEN_SEMICOLON;
                break;
            case '{':
                state->tokenType = TOKEN_BLOCK_OPEN;
                break;
            case '}':
                state->tokenType = TOKEN_BLOCK_CLOSE;
                break;
            case '$':
                state->tokenLen = 0;
                /* read the macro name into state->curtok */
                while (ConfigIsNameChar(c = fgetc(state->stream)))
                {
                    state->token[state->tokenLen] = c;
                    state->tokenLen++;

                    if (state->tokenLen >= state->tokenSize)
                    {
                        state->tokenSize += CONFIG_BUFFER_BLOCK;
                        state->token = realloc(state->token,
                                               state->tokenSize);
                    }
                }
                state->token[state->tokenLen] = '\0';
                state->tokenLen++;
                state->tokenType = TOKEN_MACRO;

                ungetc(c, state->stream);
                break;
            default:
                state->tokenType = TOKEN_UNKNOWN;
                break;
        }
    }

    /* Resize curtok to only use the bytes it needs */
    if (state->tokenLen)
    {
        state->tokenSize = state->tokenLen;
        state->token = realloc(state->token, state->tokenSize);
    }
}

static int
ConfigExpect(ConfigParserState * state, ConfigToken tokenType)
{
    return state->tokenType == tokenType;
}


static HashMap *
ConfigParseBlock(ConfigParserState * state, int level)
{
    HashMap *block = HashMapCreate();

    ConfigTokenSeek(state);

    while (ConfigExpect(state, TOKEN_NAME))
    {
        char *name = malloc(state->tokenLen + 1);

        strcpy(name, state->token);

        ConfigTokenSeek(state);
        if (ConfigExpect(state, TOKEN_VALUE) || ConfigExpect(state, TOKEN_MACRO))
        {
            ConfigDirective *directive;

            directive = malloc(sizeof(ConfigDirective));
            directive->children = NULL;
            directive->values = ArrayCreate();

            while (ConfigExpect(state, TOKEN_VALUE) ||
                   ConfigExpect(state, TOKEN_MACRO))
            {

                char *dval;
                char *dvalCpy;

                if (ConfigExpect(state, TOKEN_VALUE))
                {
                    dval = state->token;
                }
                else if (ConfigExpect(state, TOKEN_MACRO))
                {
                    dval = HashMapGet(state->macroMap, state->token);
                    if (!dval)
                    {
                        goto error;
                    }
                }
                else
                {
                    dval = NULL;   /* Should never happen */
                }

                /* dval is a pointer which is overwritten with the next
                 * token. */
                dvalCpy = malloc(strlen(dval) + 1);
                strcpy(dvalCpy, dval);

                ArrayAdd(directive->values, dvalCpy);
                ConfigTokenSeek(state);
            }

            if (ConfigExpect(state, TOKEN_BLOCK_OPEN))
            {
                /* token_seek(state); */
                directive->children = ConfigParseBlock(state, level + 1);
                if (!directive->children)
                {
                    goto error;
                }
            }

            /*
             * Append this directive to the current block,
             * overwriting a directive at this level with the same name.
             *
             * Note that if a value already exists with this name, it is
             * returned by HashMapSet() and then immediately passed to
             * ConfigDirectiveFree(). If the value does not exist, then
             * NULL is sent to ConfigDirectiveFree(), making it a no-op.
             */
            ConfigDirectiveFree(HashMapSet(block, name, directive));
        }
        else if (ConfigExpect(state, TOKEN_MACRO_ASSIGNMENT))
        {
            ConfigTokenSeek(state);
            if (ConfigExpect(state, TOKEN_VALUE))
            {
                char *valueCopy = malloc(strlen(state->token) + 1);

                strcpy(valueCopy, state->token);
                free(HashMapSet(state->macroMap, name, valueCopy));
                ConfigTokenSeek(state);
            }
            else
            {
                goto error;
            }
        }
        else
        {
            goto error;
        }

        if (!ConfigExpect(state, TOKEN_SEMICOLON))
        {
            goto error;
        }
        ConfigTokenSeek(state);
    }

    if (ConfigExpect(state, level ? TOKEN_BLOCK_CLOSE : TOKEN_EOF))
    {
        ConfigTokenSeek(state);
        return block;
    }
    else
    {
        goto error;
    }

error:
    /* Only free the very top level, because this will recurse */
    if (!level)
    {
        ConfigFree(block);
    }
    return NULL;
}

ConfigParseResult *
ConfigParse(FILE * stream)
{
    ConfigParseResult *result;
    HashMap *conf;
    ConfigParserState *state;

    result = malloc(sizeof(ConfigParseResult));
    state = ConfigParserStateCreate(stream);
    conf = ConfigParseBlock(state, 0);

    if (!conf)
    {
        result->ok = 0;
        result->data.lineNumber = state->line;
    }
    else
    {
        result->ok = 1;
        result->data.confMap = conf;
    }

    ConfigParserStateFree(state);
    return result;
}
