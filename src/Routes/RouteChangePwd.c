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
#include <Uia.h>
#include <Memory.h>
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

ROUTE_IMPL(RouteChangePwd, args)
{
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

    if (MATRIX_PATH_PARTS(args->path) != 0 ||
        HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED);
    }

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        JsonFree(request);
        return response;
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_NOT_JSON);
    }

    uiaFlows = ArrayCreate();
    ArrayAdd(uiaFlows, PasswordFlow());
    uiaResult = UiaComplete(uiaFlows, args->context,
                            args->matrixArgs->db, request, &response,
                            args->matrixArgs->config);
    UiaFlowsFree(uiaFlows);

    if (uiaResult < 0)
    {
        JsonFree(request);
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN);
    }
    else if (!uiaResult)
    {
        JsonFree(request);
        return response;
    }

    newPassword = JsonValueAsString(HashMapGet(request, "new_password"));
    if (!newPassword)
    {
        JsonFree(request);
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_BAD_JSON);
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
        JsonFree(request);
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNKNOWN_TOKEN);
    }

    UserSetPassword(user, newPassword);

    /* We might want to logout all extra devices */
    if (logoutDevices)
    {
        /* Deletes all tokens except the passed token */
        UserDeleteTokens(user, token);
    }

    UserUnlock(user);
    JsonFree(request);
    response = HashMapCreate();
    return response;
}