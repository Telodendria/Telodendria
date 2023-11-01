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

#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>

#include <User.h>
#include <Cytoplasm/Log.h>

ROUTE_IMPL(RouteAdminDeactivate, path, argp)
{
    RouteArgs *args = argp;
    HashMap *request = NULL;
    HashMap *response = NULL;

    JsonValue *val;
    char *reason = "Deactivated by admin";
    char *removedLocalpart = ArrayGet(path, 0);
    char *token;

    Db *db = args->matrixArgs->db;

    User *user = NULL;
    User *removed = NULL;

    HttpRequestMethod method = HttpRequestMethodGet(args->context);

    if ((method != HTTP_DELETE) && (method != HTTP_PUT))
    {
        char * msg = "Route only supports DELETE and PUT as for now.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }
    
    if (method == HTTP_DELETE)
    {
        request = JsonDecode(HttpServerStream(args->context));
        if (!request)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_NOT_JSON, NULL);
        }
        val = HashMapGet(request, "reason");
        if (val && JsonValueType(val) == JSON_STRING)
        {
            reason = JsonValueAsString(val);
        }
    }


    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        goto finish;
    }

    user = UserAuthenticate(db, token);
    removed = UserLock(db, removedLocalpart);
    if (!user || !removed)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    if (!(UserGetPrivileges(user) & USER_DEACTIVATE))
    {
        char * msg = "User doesn't have the DEACTIVATE privilege.";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
        goto finish;
    }
    
    if (method == HTTP_DELETE)    
    {
        UserDeactivate(removed, UserGetName(user), reason);
        response = HashMapCreate();
        
        JsonSet(response, JsonValueString(removedLocalpart), 1, "user");
        JsonSet(response, JsonValueString(reason), 1, "reason");
        JsonSet(response, JsonValueString(UserGetName(user)), 1, "banned_by");
    }
    else
    {
        UserReactivate(removed);
        HttpResponseStatus(args->context, HTTP_NO_CONTENT);
    }

finish:
    UserUnlock(user);
    UserUnlock(removed);
    JsonFree(request);
    return response;
}
