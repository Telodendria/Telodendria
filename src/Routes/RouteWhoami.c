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

ROUTE_IMPL(RouteWhoami, path, argp)
{
    RouteArgs *args = argp;
    Db *db = args->matrixArgs->db;

    HashMap *response = NULL;
    User *user = NULL;

    char *token;
    char *userID;
    char *deviceID;

    Config config;

    ConfigLock(db, &config);

    if (!config.ok)
    {
        Log(LOG_ERR, "%s", config.err);
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, config.err);
    }

    (void) path;

    /* Get the request */
    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        /* No token? */
        goto finish;
    }

    /* Authenticate with our token */
    user = UserAuthenticate(db, token);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    response = HashMapCreate();

    userID = StrConcat(4, "@", UserGetName(user), ":", config.serverName);
    deviceID = StrDuplicate(UserGetDeviceId(user));

    UserUnlock(user);

    HashMapSet(response, "device_id", JsonValueString(deviceID));
    HashMapSet(response, "user_id", JsonValueString(userID));

    Free(userID);
    Free(deviceID);

finish:
    ConfigUnlock(&config);
    return response;
}
