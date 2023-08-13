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

#include <Json.h>
#include <Str.h>
#include <User.h>

ROUTE_IMPL(RouteAliasDirectory, path, argp)
{
    RouteArgs *args = argp;
    char *alias = ArrayGet(path, 0);

    HashMap *request = NULL;
    HashMap *response;

    Db *db = args->matrixArgs->db;
    DbRef *ref;
    HashMap *aliases;
    JsonValue *val;

    char *token;
    User *user = NULL;

    /* TODO: Return HTTP 400 M_INVALID_PARAM if alias is invalid */

    ref = DbLock(db, 1, "aliases");
    if (!ref && !(ref = DbCreate(db, 1, "aliases")))
    {
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, "Unable to access alias database.");
        goto finish;
    }

    aliases = DbJson(ref);

    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_GET:
            val = JsonGet(aliases, 2, "alias", alias);
            if (val)
            {
                response = HashMapCreate();
                HashMapSet(response, "room_id", JsonValueDuplicate(HashMapGet(JsonValueAsObject(val), "id")));
                HashMapSet(response, "servers", JsonValueDuplicate(JsonGet(aliases, 3, "alias", alias, "servers")));
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_NOT_FOUND, "There is no mapped room ID for this room alias.");
            }
            break;
        case HTTP_PUT:
        case HTTP_DELETE:
            response = MatrixGetAccessToken(args->context, &token);
            if (response)
            {
                goto finish;
            }

            user = UserAuthenticate(db, token);
            if (!user)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
                goto finish;
            }

            if (HttpRequestMethodGet(args->context) == HTTP_PUT)
            {
                HashMap *newAlias;

                /* TODO: Validate alias domain and make sure it matches
                 * server name and is well formed. */

                if (JsonGet(aliases, 2, "alias", alias))
                {
                    HttpResponseStatus(args->context, HTTP_CONFLICT);
                    response = MatrixErrorCreate(M_UNKNOWN, "Room alias already exists.");
                    goto finish;
                }

                request = JsonDecode(HttpServerStream(args->context));
                if (!request)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_NOT_JSON, NULL);
                    goto finish;
                }

                if (!JsonValueAsString(HashMapGet(request, "room_id")))
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_BAD_JSON, "Missing or invalid room_id.");
                    goto finish;
                }

                /* TODO: Validate room ID to make sure it is well
                 * formed. */

                newAlias = HashMapCreate();
                HashMapSet(newAlias, "createdBy", JsonValueString(UserGetName(user)));
                HashMapSet(newAlias, "id", JsonValueDuplicate(HashMapGet(request, "room_id")));
                HashMapSet(newAlias, "servers", JsonValueArray(ArrayCreate()));

                JsonSet(aliases, JsonValueObject(newAlias), 2, "alias", alias);
            }
            else
            {
                if (!JsonGet(aliases, 2, "alias", alias))
                {
                    HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                    response = MatrixErrorCreate(M_NOT_FOUND, "Room alias not found.");
                    goto finish;
                }

                if (!(UserGetPrivileges(user) & USER_ALIAS) && !StrEquals(UserGetName(user), JsonValueAsString(JsonGet(aliases, 3, "alias", alias, "createdBy"))))
                {
                    HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
                    response = MatrixErrorCreate(M_UNAUTHORIZED, NULL);
                    goto finish;
                }

                JsonValueFree(HashMapDelete(JsonValueAsObject(HashMapGet(aliases, "alias")), alias));
            }
            response = HashMapCreate();

            break;
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, "Unknown request method.");
            goto finish;
    }

finish:
    UserUnlock(user);
    DbUnlock(db, ref);
    JsonFree(request);
    return response;
}
