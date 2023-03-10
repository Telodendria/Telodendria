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

ROUTE_IMPL(RouteWhoami, args)
{
    Db *db = args->matrixArgs->db;

    HashMap *response = NULL;
    HashMap *tokenJson = NULL;

    DbRef *ref;

    char *token;
    char *userID;
    char *deviceID;

    if (MATRIX_PATH_PARTS(args->path) != 0)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED);
    }

    /* Get the request */
    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        /* No token? */
        return response;
    }

    /* Authenticate with our token */
    if (!DbExists(db, 3, "tokens", "access", token))
    {
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        return MatrixErrorCreate(M_UNKNOWN_TOKEN);
    }

    ref = DbLock(db, 3, "tokens", "access", token);
    tokenJson = DbJson(ref);

    response = HashMapCreate();

    userID = StrConcat(4, "@",
                     JsonValueAsString(HashMapGet(tokenJson, "user")),
                       ":", args->matrixArgs->config->serverName);

    deviceID = StrDuplicate(JsonValueAsString(HashMapGet(tokenJson, "device")));

    DbUnlock(db, ref);

    HashMapSet(response, "device_id", JsonValueString(deviceID));
    HashMapSet(response, "user_id", JsonValueString(userID));

    Free(userID);
    Free(deviceID);
    return response;
}
