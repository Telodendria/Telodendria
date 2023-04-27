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
#include <HeaderParser.h>

#include <Memory.h>

#include <string.h>
#include <ctype.h>

static int
HeaderConsumeWhitespace(HeaderExpr * expr)
{
    int c;

    while (1)
    {
        c = StreamGetc(expr->state.stream);

        if (StreamEof(expr->state.stream) || StreamError(expr->state.stream))
        {
            expr->type = HP_EOF;
            expr->data.error.msg = "End of stream reached.";
            expr->data.error.lineNo = expr->state.lineNo;
            break;
        }

        if (isspace(c))
        {
            if (c == '\n')
            {
                expr->state.lineNo++;
            }
        }
        else
        {
            break;
        }
    }

    return c;
}

static char *
HeaderConsumeWord(HeaderExpr * expr)
{
    char *str = Malloc(16 * sizeof(char));
    int len = 16;
    int i;
    int c;

    if (!str)
    {
        return NULL;
    }

    c = HeaderConsumeWhitespace(expr);

    i = 0;
    str[i] = c;
    i++;

    while (!isspace(c = StreamGetc(expr->state.stream)))
    {
        if (i >= len)
        {
            len *= 2;
            str = Realloc(str, len * sizeof(char));
        }

        str[i] = c;
        i++;
    }

    if (i >= len)
    {
        len++;
        str = Realloc(str, len * sizeof(char));
    }

    str[i] = '\0';

    if (c != EOF)
    {
        StreamUngetc(expr->state.stream, c);
    }

    return str;
}

static char *
HeaderConsumeAlnum(HeaderExpr * expr)
{
    char *str = Malloc(16 * sizeof(char));
    int len = 16;
    int i;
    int c;

    if (!str)
    {
        return NULL;
    }

    c = HeaderConsumeWhitespace(expr);

    i = 0;
    str[i] = c;
    i++;

    while (isalnum(c = StreamGetc(expr->state.stream)))
    {
        if (i >= len)
        {
            len *= 2;
            str = Realloc(str, len * sizeof(char));
        }

        str[i] = c;
        i++;
    }

    if (i >= len)
    {
        len++;
        str = Realloc(str, len * sizeof(char));
    }

    str[i] = '\0';

    if (c != EOF)
    {
        StreamUngetc(expr->state.stream, c);
    }

    return str;
}

static char *
HeaderConsumeArg(HeaderExpr * expr)
{
    char *str = Malloc(16 * sizeof(char));
    int len = 16;
    int i;
    int c;
    int block = 0;

    if (!str)
    {
        return NULL;
    }

    c = HeaderConsumeWhitespace(expr);

    i = 0;
    str[i] = c;
    i++;

    while (((c = StreamGetc(expr->state.stream)) != ',' && c != ')') || block > 0)
    {
        if (i >= len)
        {
            len *= 2;
            str = Realloc(str, len * sizeof(char));
        }

        str[i] = c;
        i++;

        if (c == '(')
        {
            block++;
        }
        else if (c == ')')
        {
            block--;
        }
        else if (c == '\n')
        {
            expr->state.lineNo++;
        }
    }

    if (i >= len)
    {
        len++;
        str = Realloc(str, len * sizeof(char));
    }

    str[i] = '\0';

    if (c != EOF)
    {
        StreamUngetc(expr->state.stream, c);
    }

    return str;
}

void
HeaderParse(Stream * stream, HeaderExpr * expr)
{
    int c;

    if (!expr)
    {
        return;
    }

    if (!stream)
    {
        expr->type = HP_PARSE_ERROR;
        expr->data.error.msg = "NULL pointer to stream.";
        expr->data.error.lineNo = -1;
        return;
    }

    if (expr->type == HP_DECLARATION && expr->data.declaration.args)
    {
        size_t i;

        for (i = 0; i < ArraySize(expr->data.declaration.args); i++)
        {
            Free(ArrayGet(expr->data.declaration.args, i));
        }

        ArrayFree(expr->data.declaration.args);
    }

    expr->state.stream = stream;
    if (!expr->state.lineNo)
    {
        expr->state.lineNo = 1;
    }

    c = HeaderConsumeWhitespace(expr);

    if (StreamEof(stream) || StreamError(stream))
    {
        expr->type = HP_EOF;
        expr->data.error.msg = "End of stream reached.";
        expr->data.error.lineNo = expr->state.lineNo;
        return;
    }

    if (c == '/')
    {
        int i = 0;

        c = StreamGetc(expr->state.stream);
        if (c != '*')
        {
            expr->type = HP_SYNTAX_ERROR;
            expr->data.error.msg = "Expected comment opening.";
            expr->data.error.lineNo = expr->state.lineNo;
            return;
        }

        expr->type = HP_COMMENT;
        while (1)
        {
            if (i >= HEADER_EXPR_MAX - 1)
            {
                expr->type = HP_PARSE_ERROR;
                expr->data.error.msg = "Memory limit exceeded while parsing comment.";
                expr->data.error.lineNo = expr->state.lineNo;
                return;
            }

            c = StreamGetc(expr->state.stream);

            if (StreamEof(expr->state.stream) || StreamError(expr->state.stream))
            {
                expr->type = HP_SYNTAX_ERROR;
                expr->data.error.msg = "Unterminated comment.";
                expr->data.error.lineNo = expr->state.lineNo;
                return;
            }

            if (c == '*')
            {
                c = StreamGetc(expr->state.stream);
                if (c == '/')
                {
                    expr->data.text[i] = '\0';
                    break;
                }
                else
                {
                    expr->data.text[i] = '*';
                    i++;
                    expr->data.text[i] = c;
                    i++;
                    if (c == '\n')
                    {
                        expr->state.lineNo++;
                    }
                }
            }
            else
            {
                expr->data.text[i] = c;
                i++;

                if (c == '\n')
                {
                    expr->state.lineNo++;
                }
            }
        }
    }
    else if (c == '#')
    {
        int i = 0;
        char *word;

        expr->type = HP_PREPROCESSOR_DIRECTIVE;
        expr->data.text[i] = '#';
        i++;

        word = HeaderConsumeWord(expr);

        strncpy(expr->data.text + i, word, HEADER_EXPR_MAX - i - 1);
        i += strlen(word);

        if (strcmp(word, "include") == 0 ||
            strcmp(word, "undef") == 0 ||
            strcmp(word, "ifdef") == 0 ||
            strcmp(word, "ifndef") == 0)
        {
            /* Read one more word */
            Free(word);
            word = HeaderConsumeWord(expr);

            if (i + strlen(word) + 1 >= HEADER_EXPR_MAX)
            {
                expr->type = HP_PARSE_ERROR;
                expr->data.error.msg = "Memory limit reached parsing preprocessor directive.";
                expr->data.error.lineNo = expr->state.lineNo;
            }
            else
            {
                strncpy(expr->data.text + i + 1, word, HEADER_EXPR_MAX - i - 1);
                expr->data.text[i] = ' ';
            }

            Free(word);
        }
        else if (strcmp(word, "define") == 0 ||
                 strcmp(word, "if") == 0 ||
                 strcmp(word, "elif") == 0 ||
                 strcmp(word, "error") == 0)
        {
            Free(word);
            expr->data.text[i] = ' ';
            i++;

            while (1)
            {
                if (i >= HEADER_EXPR_MAX - 1)
                {
                    expr->type = HP_PARSE_ERROR;
                    expr->data.error.msg = "Memory limit reached parsing preprocessor directive.";
                    expr->data.error.lineNo = expr->state.lineNo;
                    return;
                }

                c = StreamGetc(expr->state.stream);

                if (StreamEof(expr->state.stream) || StreamError(expr->state.stream))
                {
                    expr->type = HP_SYNTAX_ERROR;
                    expr->data.error.msg = "Unterminated preprocessor directive.";
                    expr->data.error.lineNo = expr->state.lineNo;
                    return;
                }

                /* TODO: Handle backslash escapes */
                if (c == '\n')
                {
                    expr->data.text[i] = '\0';
                    expr->state.lineNo++;
                    break;
                }
                else
                {
                    expr->data.text[i] = c;
                    i++;
                }
            }
        }
        else if (strcmp(word, "else") == 0 ||
                 strcmp(word, "endif") == 0)
        {
            /* Read no more words, that's the whole directive */
        }
        else
        {
            Free(word);

            expr->type = HP_SYNTAX_ERROR;
            expr->data.error.msg = "Unknown preprocessor directive.";
            expr->data.error.lineNo = expr->state.lineNo;
        }
    }
    else
    {
        char *word;

        StreamUngetc(expr->state.stream, c);
        word = HeaderConsumeWord(expr);

        if (strcmp(word, "typedef") == 0)
        {
            int block = 0;
            int i = 0;

            expr->type = HP_TYPEDEF;
            strncpy(expr->data.text, word, HEADER_EXPR_MAX - 1);
            i += strlen(word);
            expr->data.text[i] = ' ';
            i++;

            while (1)
            {
                if (i >= HEADER_EXPR_MAX - 1)
                {
                    expr->type = HP_PARSE_ERROR;
                    expr->data.error.msg = "Memory limit exceeded while parsing typedef.";
                    expr->data.error.lineNo = expr->state.lineNo;
                    return;
                }

                c = StreamGetc(expr->state.stream);

                if (StreamEof(expr->state.stream) || StreamError(expr->state.stream))
                {
                    expr->type = HP_SYNTAX_ERROR;
                    expr->data.error.msg = "Unterminated typedef.";
                    expr->data.error.lineNo = expr->state.lineNo;
                    return;
                }

                expr->data.text[i] = c;
                i++;

                if (c == '{')
                {
                    block++;
                }
                else if (c == '}')
                {
                    block--;
                }
                else if (c == '\n')
                {
                    expr->state.lineNo++;
                }

                if (block <= 0 && c == ';')
                {
                    expr->data.text[i] = '\0';
                    break;
                }
            }
        }
        else if (strcmp(word, "extern") == 0)
        {
            int wordLimit = sizeof(expr->data.declaration.returnType) - 8;
            int wordLen;

            Free(word);

            word = HeaderConsumeWord(expr);
            wordLen = strlen(word);
            if (wordLen > wordLimit)
            {
                expr->type = HP_PARSE_ERROR;
                expr->data.error.msg = "Return of declaration exceeds length limit.";
                expr->data.error.lineNo = expr->state.lineNo;
            }
            else
            {
                int i = wordLen;

                expr->type = HP_DECLARATION;
                strncpy(expr->data.declaration.returnType, word, wordLimit);

                if (strcmp(word, "struct") == 0 ||
                    strcmp(word, "enum") == 0 ||
                    strcmp(word, "const") == 0)
                {
                    Free(word);
                    word = HeaderConsumeWord(expr);
                    wordLen = strlen(word);
                    expr->data.declaration.returnType[i] = ' ';
                    strncpy(expr->data.declaration.returnType + i + 1, word, wordLen + 1);
                    i += wordLen + 1;
                }

                Free(word);

                c = HeaderConsumeWhitespace(expr);
                if (c == '*')
                {
                    expr->data.declaration.returnType[i] = ' ';
                    i++;
                    expr->data.declaration.returnType[i] = '*';
                    i++;
                    while ((c = HeaderConsumeWhitespace(expr)) == '*')
                    {
                        expr->data.declaration.returnType[i] = c;
                        i++;
                    }
                }

                StreamUngetc(expr->state.stream, c);
                word = HeaderConsumeAlnum(expr);

                wordLen = strlen(word);
                wordLimit = sizeof(expr->data.declaration.name) - 1;

                if (wordLen > wordLimit)
                {
                    expr->type = HP_SYNTAX_ERROR;
                    expr->data.error.msg = "Function name too long.";
                    expr->data.error.lineNo = expr->state.lineNo;
                }
                else
                {
                    strncpy(expr->data.declaration.name, word, wordLimit);
                    Free(word);
                    word = NULL;

                    c = HeaderConsumeWhitespace(expr);
                    if (c != '(')
                    {
                        expr->type = HP_SYNTAX_ERROR;
                        expr->data.error.msg = "Expected '('";
                        expr->data.error.lineNo = expr->state.lineNo;
                        return;
                    }

                    expr->data.declaration.args = ArrayCreate();

                    do
                    {
                        word = HeaderConsumeArg(expr);
                        ArrayAdd(expr->data.declaration.args, word);
                        word = NULL;
                    }
                    while ((!StreamEof(expr->state.stream)) && ((c = HeaderConsumeWhitespace(expr)) != ')'));

                    if (StreamEof(expr->state.stream))
                    {
                        expr->type = HP_SYNTAX_ERROR;
                        expr->data.error.msg = "End of file reached before ')'.";
                        expr->data.error.lineNo = expr->state.lineNo;
                        return;
                    }

                    c = HeaderConsumeWhitespace(expr);
                    if (c != ';')
                    {
                        expr->type = HP_SYNTAX_ERROR;
                        expr->data.error.msg = "Expected ';'.";
                        expr->data.error.lineNo = expr->state.lineNo;
                        return;
                    }
                }
            }
        }
        else
        {
            expr->type = HP_SYNTAX_ERROR;
            expr->data.error.msg = "Expected comment, typedef, or extern.";
            expr->data.error.lineNo = expr->state.lineNo;
        }

        Free(word);
    }
}
