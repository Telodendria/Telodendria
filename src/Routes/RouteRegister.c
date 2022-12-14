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
#include <Memory.h>

ROUTE_IMPL(RouteRegister, args)
{
    HashMap *request = NULL;
    HashMap *response = NULL;

    char *pathPart = NULL;

    if (MATRIX_PATH_PARTS(args->path) == 0)
    {

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

        /* TODO: Complete account registration flow */

        JsonFree(request);
    }
    else
    {
        pathPart = MATRIX_PATH_POP(args->path);

        if (HttpRequestMethodGet(args->context) == HTTP_GET &&
            MATRIX_PATH_EQUALS(pathPart, "available"))
        {
            char *username = HashMapGet(HttpRequestParams(args->context), "username");

            if (!username)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
            }

            /* TODO: Check if ?username=x is available */
        }
        else if (HttpRequestMethodGet(args->context) == HTTP_POST &&
                 (MATRIX_PATH_EQUALS(pathPart, "email") ||
                  MATRIX_PATH_EQUALS(pathPart, "msisdn")))
        {
            Free(pathPart);
            pathPart = MATRIX_PATH_POP(args->path);
            if (!MATRIX_PATH_EQUALS(pathPart, "requestToken"))
            {
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_UNRECOGNIZED);
            }
            else
            {
                /* TODO: Validate request body and potentially return
                 * M_BAD_JSON */
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_THREEPID_DENIED);
            }
        }
        else
        {
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_UNRECOGNIZED);
        }

        Free(pathPart);
    }

    return response;
}
