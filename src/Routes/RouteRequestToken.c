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

#include <Str.h>
#include <Json.h>

ROUTE_IMPL(RouteRequestToken, path, argp)
{
    RouteArgs *args = argp;
    char *type = ArrayGet(path, 0);
    HashMap *request;
    HashMap *response;
    JsonValue *val;
    char *str;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, NULL);
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_NOT_JSON, NULL);
    }

    val = HashMapGet(request, "client_secret");
    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    str = JsonValueAsString(val);
    if (strlen(str) > 255 || StrBlank(str))
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    val = HashMapGet(request, "send_attempt");
    if (!val || JsonValueType(val) != JSON_INTEGER)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    val = HashMapGet(request, "next_link");
    if (val && JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    val = HashMapGet(request, "id_access_token");
    if (val && JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    val = HashMapGet(request, "id_server");
    if (val && JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, NULL);
        goto finish;
    }

    if (StrEquals(type, "email"))
    {
        val = HashMapGet(request, "email");
        if (val && JsonValueType(val) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, NULL);
            goto finish;
        }
    }
    else if (StrEquals(type, "msisdn"))
    {
        val = HashMapGet(request, "country");
        if (val && JsonValueType(val) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, NULL);
            goto finish;
        }

        str = JsonValueAsString(val);
        if (strlen(str) != 2)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, NULL);
            goto finish;
        }

        val = HashMapGet(request, "phone_number");
        if (val && JsonValueType(val) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, NULL);
            goto finish;
        }
    }
    else
    {
        /* Should not be possible */
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, NULL);
        goto finish;
    }

    HttpResponseStatus(args->context, HTTP_FORBIDDEN);
    response = MatrixErrorCreate(M_THREEPID_DENIED, NULL);

finish:
    JsonFree(request);
    return response;
}
