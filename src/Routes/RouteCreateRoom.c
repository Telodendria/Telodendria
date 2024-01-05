/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net>
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

#include <Cytoplasm/Json.h>

#include <Schema/RoomCreateRequest.h>

ROUTE_IMPL(RouteCreateRoom, path, argp)
{
    RouteArgs *args = argp;

    HashMap *request = NULL;
    HashMap *response;
    RoomCreateRequest parsed;
    char *err;

    (void) path;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        err = "Unknown request method.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNRECOGNIZED, err);
        goto finish;
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_NOT_JSON, NULL);
        goto finish;
    }

    if (!RoomCreateRequestFromJson(request, &parsed, &err))
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, err);
        goto finish;
    }

    /* No longer need this now that it is parsed */
    JsonFree(request);
    request = NULL;

    response = HashMapCreate();

finish:
    JsonFree(request);
    return response;
}
