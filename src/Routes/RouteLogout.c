/*
 * Copyright (C) 2022-2025 Jordan Bancino <@jordan:synapse.telodendria.org>
 * with other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Memory.h>
#include <User.h>

ROUTE_IMPL(RouteLogout, path, argp)
{
    RouteArgs *args = argp;
    HashMap *response = NULL;

    char *tokenstr;

    char *msg;

    Db *db = args->matrixArgs->db;

    User *user;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        msg = "This route only accepts POST.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }

    response = MatrixGetAccessToken(args->context, &tokenstr);
    if (response)
    {
        return response;
    }

    user = UserAuthenticate(db, tokenstr);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        return MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
    }

    if (ArraySize(path) == 1)
    {
        if (!StrEquals(ArrayGet(path, 0), "all"))
        {
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_NOT_FOUND, NULL);
            goto finish;
        }

        if (!UserDeleteTokens(user, NULL))
        {
            /* If we can't delete all of our tokens, then something is
             * wrong. */
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, NULL);
            goto finish;
        }
        response = HashMapCreate();
    }
    else
    {
        if (!UserDeleteToken(user, tokenstr))
        {
            msg = "Internal server error: couldn't delete token.";
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, msg);
            goto finish;
        }

        response = HashMapCreate();
    }

finish:
    UserUnlock(user);
    return response;
}
