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

#include <string.h>

#include <Json.h>
#include <HashMap.h>
#include <Str.h>
#include <Memory.h>
#include <User.h>

ROUTE_IMPL(RouteLogout, args)
{
    HashMap *response = NULL;

    char *tokenstr;

    Db *db = args->matrixArgs->db;

    User *user;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED);
    }

    if (MATRIX_PATH_PARTS(args->path) > 1)
    {
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
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
        return MatrixErrorCreate(M_UNKNOWN_TOKEN);
    }

    if (MATRIX_PATH_PARTS(args->path) == 1)
    {
        char *pathPart = MATRIX_PATH_POP(args->path);

        if (!MATRIX_PATH_EQUALS(pathPart, "all"))
        {
            Free(pathPart);
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_NOT_FOUND);
            goto finish;
        }
        Free(pathPart);

        if (!UserDeleteTokens(user))
        {
            /* If we can't delete all of our tokens, then something is
             * wrong. */
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN);
            goto finish;
        }
        response = HashMapCreate();
    }
    else
    {
        if (!UserDeleteToken(user, tokenstr))
        {
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN);
            goto finish;
        }

        response = HashMapCreate();
    }

finish:
    UserUnlock(user);
    return response;
}
