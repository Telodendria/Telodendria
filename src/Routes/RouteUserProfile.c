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

#include <HashMap.h>
#include <Memory.h>
#include <User.h>
#include <Json.h>
#include <Str.h>

ROUTE_IMPL(RouteUserProfile, path, argp)
{
    RouteArgs *args = argp;
    Db *db = args->matrixArgs->db;

    HashMap *request = NULL;
    HashMap *response = NULL;

    UserId *userId = NULL;
    User *user = NULL;

    char *serverName;
    char *username = NULL;
    char *entry = NULL;
    char *token = NULL;
    char *value = NULL;

    Config *config = ConfigLock(db);

    if (!config)
    {
        Log(LOG_ERR, "User profile endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN);
    }

    serverName = config->serverName;

    username = ArrayGet(path, 0);
    userId = UserIdParse(username, serverName);
    if (!userId)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_INVALID_PARAM);
        goto finish;
    }
    if (strcmp(userId->server, serverName))
    {
        /* TODO: Implement lookup over federation. */
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN);
        goto finish;
    }

    /* TODO: Actually implement user data */
    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_GET:
            user = UserLock(db, userId->localpart);
            if (!user)
            {
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_NOT_FOUND);
                goto finish;
            }

            if (ArraySize(path) > 1)
            {
                entry = ArrayGet(path, 1);
                response = HashMapCreate();

                value = UserGetProfile(user, entry);
                if (value)
                {
                    HashMapSet(response, entry, JsonValueString(value));
                }
                goto finish;
            }
            response = HashMapCreate();
            value = UserGetProfile(user, "avatar_url");
            if (value)
            {
                HashMapSet(response, "avatar_url", JsonValueString(value));
            }
            value = UserGetProfile(user, "displayname");
            if (value)
            {
                HashMapSet(response, "displayname", JsonValueString(value));
            }
            goto finish;
        case HTTP_PUT:
            if (ArraySize(path) > 1)
            {
                request = JsonDecode(HttpServerStream(args->context));
                if (!request)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_NOT_JSON);
                    goto finish;
                }
                response = MatrixGetAccessToken(args->context, &token);
                if (response)
                {
                    goto finish;
                }
                user = UserAuthenticate(db, token);
                if (!user)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_UNKNOWN_TOKEN);
                    goto finish;
                }
                entry = ArrayGet(path, 1);
                if (strcmp(entry, "displayname") == 0 ||
                    strcmp(entry, "avatar_url") == 0)
                {
                    /* Check if user has privilege to do that action. */
                    if (strcmp(userId->localpart, UserGetName(user)) == 0)
                    {
                        value = JsonValueAsString(HashMapGet(request, entry));
                        /* TODO: Make UserSetProfile notify other
                         * parties of the change */
                        UserSetProfile(user, entry, value);
                        response = HashMapCreate();
                        goto finish;
                    }
                    /* User is not allowed to carry-on the action */
                    HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                    response = MatrixErrorCreate(M_FORBIDDEN);
                    goto finish;
                }
                else
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_UNRECOGNIZED);
                    goto finish;
                }
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                goto finish;
            }
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNKNOWN);
            break;
    }
finish:
    ConfigUnlock(config);
    Free(username);
    Free(entry);
    UserIdFree(userId);
    UserUnlock(user);
    JsonFree(request);
    return response;
}
