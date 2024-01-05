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

#include <string.h>

#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>
#include <Uia.h>
#include <Cytoplasm/Memory.h>
#include <User.h>

static Array *
PasswordFlow(void)
{
    Array *ret = ArrayCreate();

    if (!ret)
    {
        return NULL;
    }
    ArrayAdd(ret, UiaStageBuild("m.login.password", NULL));
    return ret;
}

ROUTE_IMPL(RouteChangePwd, path, argp)
{
    RouteArgs *args = argp;
    Db *db = args->matrixArgs->db;

    User *user = NULL;

    HashMap *request = NULL;
    HashMap *response = NULL;

    JsonValue *val = NULL;

    Array *uiaFlows = NULL;

    int uiaResult;
    int logoutDevices = 1;

    char *token;
    char *newPassword;

    char *msg;

    Config *config = ConfigLock(db);

    if (!config)
    {
        Log(LOG_ERR, "Password endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, NULL);
    }

    (void) path;

    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        msg = "Route only supports POST.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
        goto finish;
    }

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
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
    ArrayAdd(uiaFlows, PasswordFlow());
    uiaResult = UiaComplete(uiaFlows, args->context,
                            db, request, &response,
                            config);
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

    newPassword = JsonValueAsString(HashMapGet(request, "new_password"));
    if (!newPassword)
    {
        msg = "'new_password' is unset or not a string.";
        JsonFree(request);
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }

    val = HashMapGet(request, "logout_devices");
    if (val)
    {
        logoutDevices = JsonValueAsBoolean(val);
    }

    /* Let's authenticate the user */
    user = UserAuthenticate(db, token);

    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    UserSetPassword(user, newPassword);

    /* We might want to logout all extra devices */
    if (logoutDevices)
    {
        /* Deletes all tokens except the passed token */
        UserDeleteTokens(user, token);
    }

    response = HashMapCreate();

finish:
    ConfigUnlock(config);
    UserUnlock(user);
    JsonFree(request);
    return response;
}
