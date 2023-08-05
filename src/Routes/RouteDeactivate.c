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

#include <Array.h>
#include <HashMap.h>
#include <Json.h>
#include <Str.h>
#include <Memory.h>
#include <User.h>
#include <Uia.h>

ROUTE_IMPL(RouteDeactivate, path, argp)
{
    RouteArgs *args = argp;
    HashMap *response = NULL;
    HashMap *request = NULL;
    int uiaResult;

    char *tokenstr = NULL;
    Array *uiaFlows;

    Db *db = args->matrixArgs->db;
    User *user = NULL;
    Config *config = ConfigLock(db);

    (void) path;

    if (!config)
    {
        Log(LOG_ERR, "Deactivate endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, NULL);
        goto finish;
    }

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
        goto finish;
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_NOT_JSON, NULL);
        goto finish;
    }

    uiaFlows = ArrayCreate();
    response = MatrixGetAccessToken(args->context, &tokenstr);

    if (response)
    {
        /* No access token provided, require password */
        Array *passwordFlow = ArrayCreate();

        ArrayAdd(passwordFlow, UiaStageBuild("m.login.password", NULL));
        ArrayAdd(uiaFlows, passwordFlow);
    }
    else
    {
        /* Access token provided, no further authentication needed. */
        ArrayAdd(uiaFlows, UiaDummyFlow());
        JsonFree(response);
    }

    uiaResult = UiaComplete(uiaFlows, args->context, db, request, &response, config);
    UiaFlowsFree(uiaFlows);

    if (uiaResult < 0)
    {
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, NULL);
        goto finish;
    }
    else if (!uiaResult)
    {
        goto finish;
    }

    if (tokenstr)
    {
        user = UserAuthenticate(db, tokenstr);
    }
    else
    {
        /* No access token, we have to get the user off UIA */
        char *session = JsonValueAsString(JsonGet(request, 2, "auth", "session"));
        DbRef *sessionRef = DbLock(db, 2, "user_interactive", session);
        char *userId = JsonValueAsString(HashMapGet(DbJson(sessionRef), "user"));

        user = UserLock(db, userId);
        DbUnlock(db, sessionRef);
    }

    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    if (!UserDeleteTokens(user, NULL) || !UserDeactivate(user))
    {
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, NULL);
        goto finish;
    }

    response = HashMapCreate();

    /*
     * "This must be success if the homeserver has no identifiers to
     * unbind for the user."
     */
    HashMapSet(response, "id_server_unbind_result", JsonValueString("success"));

finish:
    JsonFree(request);
    UserUnlock(user);
    ConfigUnlock(config);
    return response;
}
