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
#ifndef CYTOPLASM_ARGS_H
#define CYTOPLASM_ARGS_H

/***
 * @Nm Args
 * @Nd Getopt-style argument parser that operates on arrays.
 * @Dd May 12 2023
 * @Xr Array
 *
 * .Nm
 * provides a simple argument parser in the style of
 * .Xr getopt 3 .
 * It exists because the runtime passes the program arguments as
 * an Array, and it is often useful to parse the arguments to
 * provide the standard command line interface.
 */

#include <Array.h>

/**
 * All state is stored in this structure, instead of global
 * variables. This makes
 * .Nm
 * thread-safe and easy to reset.
 */
typedef struct ArgParseState
{
    int optInd;
    int optErr;
    int optOpt;
    char *optArg;

    int optPos;
} ArgParseState;

/**
 * Initialize the variables in the given parser state structure
 * to their default values. This should be called before
 * .Fn ArgParse
 * is called with the parser state. It should also be called if
 * .Fn ArgParse
 * will be used again on a different array, or the same array all
 * over again.
 */
extern void ArgParseStateInit(ArgParseState *);

/**
 * Parse command line arguments stored in the given array, using
 * the given state and option string. This function behaves
 * identically to the POSIX
 * .Fn getopt
 * function, and should be used in exactly the same way. Refer to
 * your system's
 * .Xr getopt 3
 * page for details.
 */
extern int ArgParse(ArgParseState *, Array *, const char *);

#endif /* CYTOPLASM_ARGS_H */
