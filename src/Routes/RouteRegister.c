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

#include <string.h>

#include <Json.h>
#include <HashMap.h>
#include <Util.h>

ROUTE_IMPL(RouteRegister, args)
{
    HashMap *request = NULL;
    HashMap *response = NULL;

    if (MATRIX_PATH_PARTS(args->path) > 0)
    {
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED);
    }

    if (!(args->matrixArgs->config->flags & TELODENDRIA_REGISTRATION))
    {
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        return MatrixErrorCreate(M_FORBIDDEN);
    }

    request = JsonDecode(HttpStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_NOT_JSON);
    }

    JsonFree(request);
    return response;
}
