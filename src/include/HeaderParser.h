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
#ifndef TELODENDRIA_HEADERPARSER_H
#define TELODENDRIA_HEADERPARSER_H

#include <Stream.h>
#include <Array.h>

/* Here's a comment */
#define HEADER_EXPR_MAX 4096

typedef enum HeaderExprType
{
    HP_COMMENT,
    HP_PREPROCESSOR_DIRECTIVE,
    HP_TYPEDEF,
    HP_DECLARATION,
    HP_GLOBAL,
    HP_SYNTAX_ERROR,
    HP_PARSE_ERROR,
    HP_EOF
} HeaderExprType;

typedef struct HeaderDeclaration
{
    char returnType[64];
    char name[32]; /* Enforced by ANSI C */
    Array *args;
} HeaderDeclaration;

typedef struct HeaderGlobal
{
    char type[64];
    char name[HEADER_EXPR_MAX - 64];
} HeaderGlobal;

typedef struct HeaderExpr
{
    HeaderExprType type;
    union
    {
        char text[HEADER_EXPR_MAX];
        HeaderDeclaration declaration;
        HeaderGlobal global;
        struct
        {
            int lineNo;
            char *msg;
        } error;
    } data;

    struct
    {
        Stream *stream;
        int lineNo;
    } state;
} HeaderExpr;

extern void
HeaderParse(Stream *, HeaderExpr *);

#endif /* TELODENDRIA_HEADERPARSER_H */
