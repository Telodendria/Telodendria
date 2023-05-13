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
#ifndef CYTOPLASM_HEADERPARSER_H
#define CYTOPLASM_HEADERPARSER_H

/***
 * @Nm HeaderParser
 * @Nd Parse simple C header files.
 * @Dd April 29 2023
 * 
 * .Nm
 * is an extremely simple parser that lacks most of the functionality
 * one would expect from a C code parser. It simply maps a stream
 * of tokens into a few categories, parsing major ``milestones'' in
 * a header, without actually understanding any of the details.
 * .Pp
 * This exists because it is used to generate man pages from headers.
 * See
 * .Xr hdoc 1 
 * for example usage of this parser.
 */

#include <Stream.h>
#include <Array.h>

#define HEADER_EXPR_MAX 4096

/**
 * Headers are parsed as expressions. These are the expressions that
 * this parser recognizes.
 */
typedef enum HeaderExprType
{
    HP_COMMENT,
    HP_PREPROCESSOR_DIRECTIVE,
    HP_TYPEDEF,
    HP_DECLARATION,
    HP_GLOBAL,
    HP_UNKNOWN,
    HP_SYNTAX_ERROR,
    HP_PARSE_ERROR,
    HP_EOF
} HeaderExprType;

/**
 * A representation of a function declaration.
 */
typedef struct HeaderDeclaration
{
    char returnType[64];
    char name[32]; /* Enforced by ANSI C */
    Array *args;
} HeaderDeclaration;

/**
 * A global variable declaration. The type must be of the same size
 * as the function declaration's return type due to the way parsing
 * them is implemented.
 */
typedef struct HeaderGlobal
{
    char type[64];
    char name[HEADER_EXPR_MAX - 64];
} HeaderGlobal;

/**
 * A representation of a single header expression. Note that that state
 * structure is entirely internally managed, so it should not be
 * accessed or manipulated by functions outside the functions defined
 * here.
 * .Pp
 * The type field should be used to determine which field in the data
 * union is valid.
 */
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

/**
 * Parse the next expression into the given header expression structure.
 * To parse an entire C header, this function should be called in a 
 * loop until the type of the expression is HP_EOF.
 */
extern void HeaderParse(Stream *, HeaderExpr *);

#endif /* CYTOPLASM_HEADERPARSER_H */
