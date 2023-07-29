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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <Args.h>
#include <Memory.h>
#include <Array.h>
#include <HashMap.h>
#include <Str.h>
#include <Stream.h>

#define BLOCK 64

/*
 * Anything that can be done in the language itself should; to
 * keep the interpreter minimal. Note that this has the side effect
 * of allowing the runtime to become corrupted or undefined which
 * may lead to confusing behavior, but the language primitives
 * cannot be corrupted, so things can always be corrected.
 */
static char runtime[] =
"[def let; name, value; [eval [(]def [name][;][;][value][)]]]"
"[def not; cond; [if [cond];;true]]"
"[def (); str; [(][str][)]]"
"[def #;;]"
"[def //;;]"
"[def day;; [time %d]]"
"[def month;; [time %m]]"
"[def year;; [time %Y]]"
"[def day-name;; [time %A]]"
"[def month-name;; [time %B]]"
"[def hour;; [time %I]]"
"[def minute;; [time %M]]"
"[def second;; [time %S]]"
"[def am/pm;; [time %p]]"
"[def timezone;; [time %Z]]"
"[def date;; [day-name], [month-name] [day], [year]]"
"[def timestamp;; [hour]:[minute]:[second] [am/pm] [timezone]]"
;

static Stream *err;
static int debug;

typedef struct Definition
{
    Array *args;
    char *body;
} Definition;

static char *
 EvalExpr(char *expr, Array * stack);

static char *
ReadExpr(Stream * in)
{
    size_t len = 0;
    size_t size = 0;

    size_t level = 1;

    int c;

    char *expr;
    char *tmp;

    size += BLOCK;
    expr = Malloc(size * sizeof(char));
    if (!expr)
    {
        return NULL;
    }

    while ((c = StreamGetc(in)) != EOF)
    {
        if (len + 1 >= size)
        {
            size += BLOCK;
            tmp = Realloc(expr, size * sizeof(char));
            if (!tmp)
            {
                Free(expr);
                return NULL;
            }

            expr = tmp;
        }

        if (c == ']')
        {
            if (level > 0)
            {
                level--;
            }

            if (!level)
            {
                break;
            }
        }

        if (c == '[')
        {
            level++;
        }

        expr[len] = c;
        len++;
    }

    expr[len] = '\0';
    return expr;
}

static char *
Eval(char **argp, Array * stack)
{
    size_t len;
    size_t size;

    char *res;
    char *tmp;
    char *arg = *argp;

    len = 0;
    size = BLOCK;
    res = Malloc(size * sizeof(char));
    if (!res)
    {
        return NULL;
    }

    while (*arg)
    {
        if (len + 1 >= size)
        {
            size += BLOCK;
            tmp = Realloc(res, size * sizeof(char));
            if (!tmp)
            {
                Free(res);
                return NULL;
            }

            res = tmp;
        }

        if (*arg == ';')
        {
            res[len] = '\0';
            break;
        }
        else if (*arg == '[')
        {
            size_t level = 1;
            char *endp;
            char *evalRes;
            char *base;

            arg++;
            endp = arg;

            while (level && *endp)
            {
                if (*endp == ']')
                {
                    if (level > 0)
                    {
                        level--;
                    }

                    if (!level)
                    {
                        break;
                    }
                }
                else if (*endp == '[')
                {
                    level++;
                }

                endp++;
            }

            *endp = '\0';

            evalRes = EvalExpr(arg, stack);
            base = evalRes;

            while (*evalRes)
            {
                if (len + 1 >= size)
                {
                    size += BLOCK;
                    tmp = Realloc(res, size * sizeof(char));
                    if (!tmp)
                    {
                        Free(base);
                        Free(res);
                        return NULL;
                    }

                    res = tmp;
                }

                res[len] = *evalRes;
                len++;
                evalRes++;
            }

            Free(base);
            *endp = ']';
            arg = endp;
        }
        else
        {
            res[len] = *arg;
            len++;
        }

        arg++;
    }

    if (*arg == ';')
    {
        arg++;
        while (isspace(*arg))
        {
            arg++;
        }
    }

    res[len] = '\0';
    *argp = arg;

    return res;
}

static char *
EvalExpr(char *expr, Array * stack)
{
    char *argv = expr;
    char *op = expr;
    char ops;
    char *opi;

    char *res;

    while (*argv && !isspace(*argv))
    {
        argv++;
    }

    ops = *argv;
    opi = argv;

    *argv = '\0';
    argv++;


    if (StrEquals(op, "("))
    {
        res = StrDuplicate("[");
    }
    else if (StrEquals(op, ")"))
    {
        res = StrDuplicate("]");
    }
    else if (StrEquals(op, ";"))
    {
        res = StrDuplicate(";");
    }
    else if (StrEquals(op, "env"))
    {
        char *env = Eval(&argv, stack);
        char *val = getenv(env);

        Free(env);

        if (!val)
        {
            val = "";
        }

        res = StrDuplicate(val);
    }
    else if (StrEquals(op, "time"))
    {
        char *fmt = Eval(&argv, stack);

        time_t currentTime = time(NULL);
        struct tm *timeInfo = localtime(&currentTime);
        char *timestamp = Malloc(128 * sizeof(char));

        if (strftime(timestamp, 128 * sizeof(char), fmt, timeInfo))
        {
            res = timestamp;
        }
        else
        {
            res = StrDuplicate("");
            Free(timestamp);
        }

        Free(fmt);
    }
    else if (StrEquals(op, "eval"))
    {
        char *expr = Eval(&argv, stack);
        char *tmp = expr;

        res = Eval(&tmp, stack);
        Free(expr);
    }
    else if (StrEquals(op, "if"))
    {
        char *cond = Eval(&argv, stack);

        /*
         * If the condition is false, seek past the true argument
         * without evaluating it.
         */
        if (StrBlank(cond))
        {
            size_t level = 0;

            while (*argv)
            {
                switch (*argv)
                {
                    case '[':
                        level++;
                        break;
                    case ']':
                        if (level > 0)
                        {
                            level--;
                        }
                        break;
                    case ';':
                        if (!level)
                        {
                            argv++;
                            while (isspace(*argv))
                            {
                                argv++;
                            }
                            goto end_loop;
                        }
                        break;
                }

                argv++;
            }
        }
end_loop:
        res = Eval(&argv, stack);
        Free(cond);
    }
    else if (StrEquals(op, "eq?"))
    {
        char *s1 = Eval(&argv, stack);
        char *s2 = Eval(&argv, stack);

        if (StrEquals(s1, s2))
        {
            res = StrDuplicate("true");
        }
        else
        {
            res = StrDuplicate("");
        }

        Free(s1);
        Free(s2);
    }
    else if (StrEquals(op, "def?"))
    {
        Definition *def;
        size_t i;
        char *directive = Eval(&argv, stack);

        for (i = 0; i < ArraySize(stack); i++)
        {
            HashMap *stackFrame = ArrayGet(stack, i);

            def = HashMapGet(stackFrame, directive);

            if (def)
            {
                break;
            }
        }

        if (def)
        {
            res = StrDuplicate("true");
        }
        else
        {
            res = StrDuplicate("");
        }

        Free(directive);
    }
    else if (StrEquals(op, "print"))
    {
        char *msg = Eval(&argv, stack);

        StreamPrintf(err, msg);
        Free(msg);
        res = StrDuplicate("");
    }
    else if (StrEquals(op, "def"))
    {
        char *name = Eval(&argv, stack);
        char *args = Eval(&argv, stack);
        char *body = argv;

        char *tok;
        HashMap *stackFrame;
        size_t i;

        Definition *def = Malloc(sizeof(Definition));

        def->body = StrDuplicate(body);
        def->args = ArrayCreate();

        if (debug)
        {
            StreamPrintf(err, "(def op = '%s', args = [", name);
        }

        tok = strtok(args, ",");
        while (tok)
        {
            while (isspace(*tok))
            {
                tok++;
            }

            ArrayAdd(def->args, StrDuplicate(tok));

            if (debug)
            {
                StreamPrintf(err, "'%s', ", tok);
            }

            tok = strtok(NULL, ",");
        }

        if (debug)
        {
            StreamPrintf(err, "], body = '%s')\n", def->body);
        }

        Free(args);
        stackFrame = ArrayGet(stack, ArraySize(stack) - 1);
        def = HashMapSet(stackFrame, name, def);

        if (def)
        {
            for (i = 0; i < ArraySize(def->args); i++)
            {
                Free(ArrayGet(def->args, i));
            }

            ArrayFree(def->args);
            Free(def->body);
            Free(def);
        }

        Free(name);
        res = StrDuplicate("");
    }
    else if (StrEquals(op, "undef"))
    {
        char *name = Eval(&argv, stack);
        HashMap *stackFrame = ArrayGet(stack, ArraySize(stack) - 1);
        Definition *def = HashMapDelete(stackFrame, name);

        Free(name);

        if (def)
        {
            size_t i;

            for (i = 0; i < ArraySize(def->args); i++)
            {
                Free(ArrayGet(def->args, i));
            }

            ArrayFree(def->args);
            Free(def->body);
            Free(def);
        }

        res = StrDuplicate("");
    }
    else if (StrEquals(op, "include"))
    {
        char *fileName = Eval(&argv, stack);
        Stream *file = StreamOpen(fileName, "r");

        size_t size;
        size_t len;
        char c;

        Free(fileName);
        if (!file)
        {
            res = StrDuplicate("");
            goto finish;
        }

        size = BLOCK;
        len = 0;
        res = Malloc(size * sizeof(char));

        while ((c = StreamGetc(file)) != EOF)
        {
            if (len + 1 >= size)
            {
                size += BLOCK;
                res = Realloc(res, size * sizeof(char));
            }

            res[len] = c;
            len++;
        }

        res[len] = '\0';
        StreamClose(file);
    }
    else
    {
        /* Locate macro on stack and execute it */
        Definition *def;
        char *body;
        HashMap *stackFrame;
        size_t i;

        for (i = 0; i < ArraySize(stack); i++)
        {
            stackFrame = ArrayGet(stack, i);
            def = HashMapGet(stackFrame, op);

            if (def)
            {
                break;
            }
        }

        if (!def)
        {
            res = StrDuplicate("");
            goto finish;
        }

        stackFrame = HashMapCreate();
        for (i = 0; i < ArraySize(def->args); i++)
        {
            char *argName = ArrayGet(def->args, i);
            char *argVal = Eval(&argv, stack);
            Definition *argDef = Malloc(sizeof(Definition));

            argDef->args = NULL;
            argDef->body = argVal;
            HashMapSet(stackFrame, argName, argDef);
        }

        ArrayInsert(stack, 0, stackFrame);
        body = def->body;
        res = Eval(&body, stack);

        ArrayDelete(stack, 0);

        while (HashMapIterate(stackFrame, &body, (void **) &def))
        {
            for (i = 0; i < ArraySize(def->args); i++)
            {
                Free(ArrayGet(def->args, i));
            }
            ArrayFree(def->args);
            Free(def->body);
            Free(def);
        }
        HashMapFree(stackFrame);
    }

finish:
    *opi = ops;
    return res;
}

static int
Process(Stream * in, Stream * out, Array * stack)
{
    int c;

    while ((c = StreamGetc(in)) != EOF)
    {
        if (c == '[')
        {
            char *expr;
            char *res;

            expr = ReadExpr(in);
            res = EvalExpr(expr, stack);

            StreamPuts(out, res);
            StreamFlush(out);

            Free(expr);
            Free(res);
        }
        else
        {
            StreamPutc(out, c);
        }
        StreamFlush(out);
    }

    return 1;
}

int
Main(Array * args)
{
    ArgParseState arg;
    int opt;
    int ret = EXIT_FAILURE;

    size_t i;
    Array *stack;

    char *rt;

    Stream *in;
    Stream *out;
    Array *eval = ArrayCreate();

    debug = 0;
    in = StreamStdin();
    out = StreamStdout();
    err = StreamStderr();

    ArgParseStateInit(&arg);
    while ((opt = ArgParse(&arg, args, "i:o:e:d")) != -1)
    {
        switch (opt)
        {
            case 'i':
                if (StrEquals(arg.optArg, "-"))
                {
                    in = StreamStdin();
                }

                in = StreamOpen(arg.optArg, "r");
                if (!in)
                {
                    StreamPrintf(err, "Unable to open '%s' for reading\n", arg.optArg);
                    goto finish;
                }

                break;
            case 'o':
                if (StrEquals(arg.optArg, "-"))
                {
                    out = StreamStdout();
                }

                out = StreamOpen(arg.optArg, "w");
                if (!out)
                {
                    StreamPrintf(err, "Unable to open '%s' for writing.\n", arg.optArg);
                    goto finish;
                }
                break;
            case 'e':
                ArrayAdd(eval, StrDuplicate(arg.optArg));
                break;
            case 'd':
                debug = 1;
                break;
            default:
                goto finish;
        }
    }

    stack = ArrayCreate();
    ArrayAdd(stack, HashMapCreate());

    /*
     * Evaluate the runtime environment, discarding any
     * output because the purpose is only to set the initial
     * state, not to produce extra output.
     */
    rt = runtime;
    Free(Eval(&rt, stack));

    /*
     * Evaluate any expressions specified on the command line,
     * interpolating them in the output if they produce anything
     * because that is the expected behavior.
     */
    for (i = 0; i < ArraySize(eval); i++)
    {
        char *expr = ArrayGet(eval, i);
        char *res = Eval(&expr, stack);

        StreamPuts(out, res);
        Free(res);
    }

    Process(in, out, stack);

    for (i = 0; i < ArraySize(stack); i++)
    {
        HashMap *stackFrame = ArrayGet(stack, i);
        char *key;
        Definition *val;

        while (HashMapIterate(stackFrame, &key, (void **) &val))
        {
            size_t j;

            for (j = 0; j < ArraySize(val->args); j++)
            {
                Free(ArrayGet(val->args, j));
            }

            ArrayFree(val->args);

            Free(val->body);
            Free(val);
        }

        HashMapFree(stackFrame);
    }

    ArrayFree(stack);

    ret = EXIT_SUCCESS;

finish:
    for (i = 0; i < ArraySize(eval); i++)
    {
        Free(ArrayGet(eval, i));
    }
    ArrayFree(eval);

    if (in != StreamStdin())
    {
        StreamClose(in);
    }

    if (out != StreamStdout())
    {
        StreamClose(out);
    }

    return ret;
}
