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
#include <Args.h>

#include <Memory.h>
#include <Log.h>
#include <Str.h>

#include <ctype.h>
#include <string.h>

void
ArgParseStateInit(ArgParseState * state)
{
    state->optPos = 1;
    state->optErr = 1;
    state->optInd = 1;
    state->optOpt = 0;
    state->optArg = NULL;
}

int
ArgParse(ArgParseState * state, Array * args, const char *optStr)
{
    const char *arg;

    arg = ArrayGet(args, state->optInd);

    if (arg && StrEquals(arg, "--"))
    {
        state->optInd++;
        return -1;
    }
    else if (!arg || arg[0] != '-' || !isalnum((unsigned char) arg[1]))
    {
        return -1;
    }
    else
    {
        const char *opt = strchr(optStr, arg[state->optPos]);

        state->optOpt = arg[state->optPos];

        if (!opt)
        {
            if (state->optErr && *optStr != ':')
            {
                Log(LOG_ERR, "Illegal option: %c", ArrayGet(args, 0), state->optOpt);
            }
            if (!arg[++state->optPos])
            {
                state->optInd++;
                state->optPos = 1;
            }
            return '?';
        }
        else if (opt[1] == ':')
        {
            if (arg[state->optPos + 1])
            {
                state->optArg = (char *) arg + state->optPos + 1;
                state->optInd++;
                state->optPos = 1;
                return state->optOpt;
            }
            else if (ArrayGet(args, state->optInd + 1))
            {
                state->optArg = (char *) ArrayGet(args, state->optInd + 1);
                state->optInd += 2;
                state->optPos = 1;
                return state->optOpt;
            }
            else
            {
                if (state->optErr && *optStr != ':')
                {
                    Log(LOG_ERR, "Option requires an argument: %c", state->optOpt);
                }
                if (!arg[++state->optPos])
                {
                    state->optInd++;
                    state->optPos = 1;
                }
                return *optStr == ':' ? ':' : '?';
            }
        }
        else
        {
            if (!arg[++state->optPos])
            {
                state->optInd++;
                state->optPos = 1;
            }
            return state->optOpt;
        }
    }
}
