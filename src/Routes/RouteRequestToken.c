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

#include <Cytoplasm/Str.h>
#include <Cytoplasm/Json.h>

#include <Schema/RequestToken.h>

ROUTE_IMPL(RouteRequestToken, path, argp)
{
    RouteArgs *args = argp;
    char *type = ArrayGet(path, 0);
    HashMap *request;
    HashMap *response;

    char *msg;

    RequestToken reqTok;

    Int64 minusOne = Int64Neg(Int64Create(0, 1));

    reqTok.client_secret = NULL;
    reqTok.next_link = NULL;
    reqTok.id_access_token = NULL;
    reqTok.id_server = NULL;

    reqTok.email = NULL;
    reqTok.country = NULL;
    reqTok.phone_number = NULL;

    reqTok.send_attempt = minusOne;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        msg = "This route only accepts POST.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_NOT_JSON, NULL);
    }

    if (!RequestTokenFromJson(request, &reqTok, &msg))
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    if (!reqTok.client_secret)
    {
        msg = "'client_secret' is not set";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    if (strlen(reqTok.client_secret) > 255 || StrBlank(reqTok.client_secret))
    {
        msg = "'client_secret' is blank or too long";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    if (Int64Eq(reqTok.send_attempt, minusOne))
    {
        msg = "Invalid or inexistent 'send_attempt'";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    if (!reqTok.next_link)
    {
        msg = "'next_link' is not set";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }
    if (!reqTok.id_access_token)
    {
        msg = "'id_access_token' is not set";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }
    if (!reqTok.id_server)
    {
        msg = "'id_server' is not set";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    if (StrEquals(type, "email"))
    {
        if (!reqTok.email)
        {
            msg = "Type is set to 'email' yet none was set";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, msg);
            goto finish;
        }
    }
    else if (StrEquals(type, "msisdn"))
    {
        if (!reqTok.country)
        {
            msg = "Type is set to 'msisdn' yet no country is set";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, msg);
            goto finish;
        }

        if (strlen(reqTok.country) != 2)
        {
            msg = "Invalid country tag, length must be 2";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, msg);
            goto finish;
        }

        if (!reqTok.phone_number)
        {
            msg = "Type is set to 'msisdn' yet phone_number is unset";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON, msg);
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
    RequestTokenFree(&reqTok);
    return response;
}
