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

#include <User.h>
#include <Cytoplasm/Memory.h>

#include <string.h>

ROUTE_IMPL(RoutePrivileges, path, argp)
{
    RouteArgs *args = argp;
    HashMap *response;
    char *token;
    User *user = NULL;

    HashMap *request = NULL;
    JsonValue *val;
    int privileges;

    char *msg;

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        goto finish;
    }

    user = UserAuthenticate(args->matrixArgs->db, token);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    if (!(UserGetPrivileges(user) & USER_GRANT_PRIVILEGES))
    {
        msg = "User doesn't have the GRANT_PRIVILEGES privilege";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
        goto finish;
    }

    /* If a user was specified in the URL, switch to that user after
     * verifying that the current user has privileges to do so */
    if (ArraySize(path) == 1)
    {
        UserUnlock(user);
        user = UserLock(args->matrixArgs->db, ArrayGet(path, 0));
        if (!user)
        {
            msg = "Unknown user.";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_INVALID_PARAM, msg);
            goto finish;
        }
    }

    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_POST:
        case HTTP_PUT:
        case HTTP_DELETE:
            request = JsonDecode(HttpServerStream(args->context));
            if (!request)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_NOT_JSON, NULL);
                break;
            }

            val = HashMapGet(request, "privileges");
            if (!val || JsonValueType(val) != JSON_ARRAY)
            {
                msg = "'privileges' is unset or not an array.";
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, msg);
                break;
            }

            switch (HttpRequestMethodGet(args->context))
            {
                case HTTP_POST:
                    privileges = UserDecodePrivileges(JsonValueAsArray(val));
                    break;
                case HTTP_PUT:
                    privileges = UserGetPrivileges(user);
                    privileges |= UserDecodePrivileges(JsonValueAsArray(val));
                    break;
                case HTTP_DELETE:
                    privileges = UserGetPrivileges(user);
                    privileges &= ~UserDecodePrivileges(JsonValueAsArray(val));
                    break;
                default:
                    /* Impossible */
                    privileges = USER_NONE;
                    break;
            }

            if (!UserSetPrivileges(user, privileges))
            {
                msg = "Internal server error: couldn't set privileges.";
                HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
                response = MatrixErrorCreate(M_UNKNOWN, msg);
                break;
            }

            /* Fall through */
        case HTTP_GET:
            response = HashMapCreate();
            HashMapSet(response, "privileges", JsonValueArray(UserEncodePrivileges(UserGetPrivileges(user))));
            break;
        default:
            msg = "Route only accepts POST, PUT, DELETE, and GET.";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
            goto finish;
            break;
    }

finish:
    UserUnlock(user);
    JsonFree(request);
    return response;
}
