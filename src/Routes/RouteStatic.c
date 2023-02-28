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
#include <Routes.h>
#include <Static.h>
#include <Memory.h>

ROUTE_IMPL(RouteStatic, args)
{
    FILE *stream = HttpStream(args->context);
    char *pathPart = MATRIX_PATH_POP(args->path);

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);

    if (!pathPart)
    {
        StaticItWorks(stream);
    }
    else if (MATRIX_PATH_EQUALS(pathPart, "client"))
    {
        Free(pathPart);
        pathPart = MATRIX_PATH_POP(args->path);

        if (MATRIX_PATH_EQUALS(pathPart, "login"))
        {
            StaticLogin(stream);
        }
        else
        {
            StaticError(stream, HTTP_NOT_FOUND);
        }
    }
    else
    {
        StaticError(stream, HTTP_NOT_FOUND);
    }

    Free(pathPart);
    return NULL;
}