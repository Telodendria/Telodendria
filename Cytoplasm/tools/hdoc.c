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

#include <HashMap.h>
#include <Str.h>
#include <Memory.h>
#include <Args.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include <errno.h>

typedef struct DocDecl
{
    char docs[HEADER_EXPR_MAX];
    HeaderDeclaration decl;
} DocDecl;

typedef struct DocTypedef
{
    char docs[HEADER_EXPR_MAX];
    char text[HEADER_EXPR_MAX];
} DocTypedef;

typedef struct DocGlobal
{
    char docs[HEADER_EXPR_MAX];
    HeaderGlobal global;
} DocGlobal;

static void
ParseMainBlock(HashMap * registers, Array * descr, char *comment)
{
    char *line = strtok(comment, "\n");

    while (line)
    {
        while (*line && (isspace(*line) || *line == '*'))
        {
            line++;
        }

        if (!*line)
        {
            goto next;
        }

        if (*line == '@')
        {
            int i = 0;

            line++;

            while (!isspace(line[i]))
            {
                i++;
            }

            line[i] = '\0';

            Free(HashMapSet(registers, line, StrDuplicate(line + i + 1)));
        }
        else
        {
            ArrayAdd(descr, StrDuplicate(line));
        }

next:
        line = strtok(NULL, "\n");
    }
}

int
Main(Array * args)
{
    HeaderExpr expr;
    size_t i;
    char *key;
    char *val;
    int exit = EXIT_SUCCESS;
    ArgParseState arg;

    HashMap *registers = HashMapCreate();
    Array *descr = ArrayCreate();

    Array *declarations = ArrayCreate();
    DocDecl *decl = NULL;

    Array *typedefs = ArrayCreate();
    DocTypedef *type = NULL;

    Array *globals = ArrayCreate();
    DocGlobal *global = NULL;

    char comment[HEADER_EXPR_MAX];
    int isDocumented = 0;

    Stream *in = NULL;
    Stream *out = NULL;

    int opt;

    ArgParseStateInit(&arg);
    while ((opt = ArgParse(&arg, args, "i:o:D:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                if (in)
                {
                    break;
                }

                if (StrEquals(arg.optArg, "-"))
                {
                    in = StreamStdin();
                }
                else
                {
                    int len = strlen(arg.optArg);

                    in = StreamOpen(arg.optArg, "r");
                    if (!in)
                    {
                        StreamPrintf(StreamStderr(), "Error: %s:%s",
                                     arg.optArg, strerror(errno));
                        exit = EXIT_FAILURE;
                        goto finish;
                    }

                    while (arg.optArg[len - 1] != '.')
                    {
                        arg.optArg[len - 1] = '\0';
                        len--;
                    }

                    arg.optArg[len - 1] = '\0';
                    len--;

                    HashMapSet(registers, "Nm", StrDuplicate(arg.optArg));
                }
                break;
            case 'o':
                if (out)
                {
                    break;
                }

                if (StrEquals(arg.optArg, "-"))
                {
                    out = StreamStdout();
                }
                else
                {
                    out = StreamOpen(arg.optArg, "w");
                    if (!out)
                    {
                        StreamPrintf(StreamStderr(), "Error: %s:%s",
                                     arg.optArg, strerror(errno));
                        exit = EXIT_FAILURE;
                        goto finish;
                    }
                }
                break;
            case 'D':
                val = arg.optArg;
                while (*val && *val != '=')
                {
                    val++;
                }
                if (!*val || *val != '=')
                {
                    StreamPrintf(StreamStderr(), "Bad register definition: %s",
                                 arg.optArg);
                    exit = EXIT_FAILURE;
                    goto finish;
                }

                *val = '\0';
                val++;
                HashMapSet(registers, arg.optArg, StrDuplicate(val));
                break;
        }
    }

    if (!in)
    {
        in = StreamStdin();
    }

    if (!out)
    {
        out = StreamStdout();
    }

    memset(&expr, 0, sizeof(expr));

    while (1)
    {
        HeaderParse(in, &expr);

        switch (expr.type)
        {
            case HP_PREPROCESSOR_DIRECTIVE:
                /* Ignore */
                break;
            case HP_EOF:
                /* Done parsing */
                goto last;
            case HP_PARSE_ERROR:
            case HP_SYNTAX_ERROR:
                StreamPrintf(StreamStderr(), "Parse Error: (line %d) %s\n",
                         expr.data.error.lineNo, expr.data.error.msg);
                exit = EXIT_FAILURE;
                goto finish;
            case HP_COMMENT:
                if (expr.data.text[0] != '*')
                {
                    break;
                }

                if (strncmp(expr.data.text, "**", 2) == 0)
                {
                    ParseMainBlock(registers, descr, expr.data.text);
                }
                else
                {
                    strncpy(comment, expr.data.text, sizeof(comment));
                    isDocumented = 1;
                }
                break;
            case HP_TYPEDEF:
                if (HashMapGet(registers, "ignore-typedefs"))
                {
                    break;
                }

                if (!isDocumented)
                {
                    StreamPrintf(StreamStderr(),
                                 "Error: Undocumented typedef:\n%s\n",
                                 expr.data.text);
                    exit = EXIT_FAILURE;
                    goto finish;
                }
                else
                {
                    type = Malloc(sizeof(DocTypedef));
                    strncpy(type->docs, comment, sizeof(type->docs));
                    strncpy(type->text, expr.data.text, sizeof(type->text));
                    ArrayAdd(typedefs, type);
                    isDocumented = 0;
                }
                break;
            case HP_DECLARATION:
                if (!isDocumented)
                {
                    StreamPrintf(StreamStderr(),
                                 "Error: %s() is undocumented.\n",
                                 expr.data.declaration.name);
                    exit = EXIT_FAILURE;
                    goto finish;
                }
                else
                {
                    decl = Malloc(sizeof(DocDecl));
                    decl->decl = expr.data.declaration;
                    decl->decl.args = ArrayCreate();
                    strncpy(decl->docs, comment, sizeof(decl->docs));
                    for (i = 0; i < ArraySize(expr.data.declaration.args); i++)
                    {
                        ArrayAdd(decl->decl.args, StrDuplicate(ArrayGet(expr.data.declaration.args, i)));
                    }
                    ArrayAdd(declarations, decl);
                    isDocumented = 0;
                }
                break;
            case HP_GLOBAL:
                if (!isDocumented)
                {
                    StreamPrintf(StreamStderr(),
                                 "Error: Global %s is undocumented.\n",
                                 expr.data.global.name);
                    exit = EXIT_FAILURE;
                    goto finish;
                }
                else
                {
                    global = Malloc(sizeof(DocGlobal));
                    global->global = expr.data.global;

                    strncpy(global->docs, comment, sizeof(global->docs));
                    ArrayAdd(globals, global);
                    isDocumented = 0;
                }
                break;
            case HP_UNKNOWN:
                if (HashMapGet(registers, "suppress-warnings"))
                {
                    break;
                }
                StreamPrintf(StreamStderr(), "Warning: Unknown expression: %s\n",
                             expr.data.text);
                break;
            default:
                StreamPrintf(StreamStderr(), "Unknown header type: %d\n", expr.type);
                StreamPrintf(StreamStderr(), "This is a programming error.\n");
                exit = EXIT_FAILURE;
                goto finish;
        }
    }

last:
    val = HashMapGet(registers, "Nm");
    if (!val)
    {
        HashMapSet(registers, "Nm", StrDuplicate("Unnamed"));
    }

    val = HashMapGet(registers, "Dd");
    if (!val)
    {
        time_t currentTime;
        struct tm *timeInfo;
        char tsBuf[1024];

        currentTime = time(NULL);
        timeInfo = localtime(&currentTime);
        strftime(tsBuf, sizeof(tsBuf), "%B %d %Y", timeInfo);

        val = tsBuf;
    }
    StreamPrintf(out, ".Dd $%s: %s $\n", "Mdocdate", val);

    val = HashMapGet(registers, "Os");
    if (val)
    {
        StreamPrintf(out, ".Os %s\n", val);
    }

    val = HashMapGet(registers, "Nm");
    StreamPrintf(out, ".Dt %s 3\n", val);
    StreamPrintf(out, ".Sh NAME\n");
    StreamPrintf(out, ".Nm %s\n", val);

    val = HashMapGet(registers, "Nd");
    if (!val)
    {
        val = "No Description.";
    }
    StreamPrintf(out, ".Nd %s\n", val);

    StreamPrintf(out, ".Sh SYNOPSIS\n");
    val = HashMapGet(registers, "Nm");
    StreamPrintf(out, ".In %s.h\n", val);
    for (i = 0; i < ArraySize(declarations); i++)
    {
        size_t j;

        decl = ArrayGet(declarations, i);
        StreamPrintf(out, ".Ft %s\n", decl->decl.returnType);
        StreamPrintf(out, ".Fn %s ", decl->decl.name);
        for (j = 0; j < ArraySize(decl->decl.args); j++)
        {
            StreamPrintf(out, "\"%s\" ", ArrayGet(decl->decl.args, j));
        }
        StreamPutc(out, '\n');
    }

    if (ArraySize(globals))
    {
        StreamPrintf(out, ".Sh GLOBALS\n");
        for (i = 0; i < ArraySize(globals); i++)
        {
            char *line;
            global = ArrayGet(globals, i);

            StreamPrintf(out, ".Ss %s %s\n", global->global.type, global->global.name);

            line = strtok(global->docs, "\n");
            while (line)
            {
                while (*line && (isspace(*line) || *line == '*'))
                {
                    line++;
                }

                if (*line)
                {
                    StreamPrintf(out, "%s\n", line);
                }

                line = strtok(NULL, "\n");
            }
        }
    }

    if (ArraySize(typedefs))
    {
        StreamPrintf(out, ".Sh TYPE DECLARATIONS\n");
        for (i = 0; i < ArraySize(typedefs); i++)
        {
            char *line;

            type = ArrayGet(typedefs, i);
            StreamPrintf(out, ".Bd -literal -offset indent\n");
            StreamPrintf(out, "%s\n", type->text);
            StreamPrintf(out, ".Ed\n.Pp\n");

            line = strtok(type->docs, "\n");
            while (line)
            {
                while (*line && (isspace(*line) || *line == '*'))
                {
                    line++;
                }

                if (*line)
                {
                    StreamPrintf(out, "%s\n", line);
                }

                line = strtok(NULL, "\n");
            }
        }
    }

    StreamPrintf(out, ".Sh DESCRIPTION\n");
    for (i = 0; i < ArraySize(descr); i++)
    {
        StreamPrintf(out, "%s\n", ArrayGet(descr, i));
    }

    for (i = 0; i < ArraySize(declarations); i++)
    {
        size_t j;
        char *line;

        decl = ArrayGet(declarations, i);
        StreamPrintf(out, ".Ss %s %s(",
                     decl->decl.returnType, decl->decl.name);
        for (j = 0; j < ArraySize(decl->decl.args); j++)
        {
            StreamPrintf(out, "%s", ArrayGet(decl->decl.args, j));
            if (j < ArraySize(decl->decl.args) - 1)
            {
                StreamPuts(out, ", ");
            }
        }
        StreamPuts(out, ")\n");

        line = strtok(decl->docs, "\n");
        while (line)
        {
            while (*line && (isspace(*line) || *line == '*'))
            {
                line++;
            }

            if (*line)
            {
                StreamPrintf(out, "%s\n", line);
            }

            line = strtok(NULL, "\n");
        }
    }

    val = HashMapGet(registers, "Xr");
    if (val)
    {
        char *xr = strtok(val, " ");

        StreamPrintf(out, ".Sh SEE ALSO\n");
        while (xr)
        {
            if (*xr)
            {
                StreamPrintf(out, ".Xr %s 3 ", xr);
            }

            xr = strtok(NULL, " ");

            if (xr)
            {
                StreamPutc(out, ',');
            }
            StreamPutc(out, '\n');
        }
    }

finish:
    if (in != StreamStdin())
    {
        StreamClose(in);
    }

    if (out != StreamStdout())
    {
        StreamClose(out);
    }

    for (i = 0; i < ArraySize(declarations); i++)
    {
        Free(ArrayGet(declarations, i));
    }

    for (i = 0; i < ArraySize(typedefs); i++)
    {
        Free(ArrayGet(typedefs, i));
    }

    for (i = 0; i < ArraySize(globals); i++)
    {
        Free(ArrayGet(globals, i));
    }

    for (i = 0; i < ArraySize(descr); i++)
    {
        Free(ArrayGet(descr, i));
    }

    while (HashMapIterate(registers, &key, (void **) &val))
    {
        Free(val);
    }

    HashMapFree(registers);

    return exit;
}
