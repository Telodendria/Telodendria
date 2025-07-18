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

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Db.h>

#include <Matrix.h>
#include <User.h>

ROUTE_IMPL(RouteRoomAliases, path, argp)
{
    RouteArgs *args = argp;
    char *roomId = ArrayGet(path, 0);
    char *token;
    char *msg;

    HashMap *response = NULL;
    HashMap *aliases = NULL;
    HashMap *reversealias = NULL;

    JsonValue *val;

    Db *db = args->matrixArgs->db;
    DbRef *ref = NULL;

    User *user = NULL;

    if (HttpRequestMethodGet(args->context) != HTTP_GET)
    {
        msg = "Route only accepts GET.";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
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
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    /* TODO: Check whenever the user is in the room or if its world readable 
     * once this is implemented instead of just checking for the ALIAS 
     * privilege. */
    if (!(UserGetPrivileges(user) & USER_ALIAS))
    {
        msg = "User is not allowed to get this room's aliases.";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
        goto finish;
    }

    ref = DbLock(db, 1, "aliases");
    aliases = DbJson(ref);
    reversealias = JsonValueAsObject(JsonGet(aliases, 2, "id", roomId));
    if (!reversealias)
    {
        /* We do not know about the room ID. */
        msg = "Unknown room ID.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_INVALID_PARAM, msg);
        goto finish;
    }

    response = HashMapCreate();
    val = JsonGet(reversealias, 1, "aliases");
    HashMapSet(response, "aliases", JsonValueDuplicate(val));
finish:
    DbUnlock(db, ref);
    UserUnlock(user);
    return response;
}
