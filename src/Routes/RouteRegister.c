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
#include <Uia.h>

ROUTE_IMPL(RouteRegister, args)
{
    HashMap *request = NULL;
    HashMap *response = NULL;

    JsonValue *val;

    char *kind;

    char *username = NULL;
    char *password = NULL;
    char *initialDeviceDisplayName = NULL;
    int refreshToken = 0;
    int inhibitLogin = 0;
    char *deviceId = NULL;

    Db *db = args->matrixArgs->db;
    LogConfig *lc = args->matrixArgs->lc;

    User *user = NULL;

    Array *uiaFlows;
    int uiaResult;

    if (MATRIX_PATH_PARTS(args->path) == 0)
    {
        if (HttpRequestMethodGet(args->context) != HTTP_POST)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_UNRECOGNIZED);
        }

        request = JsonDecode(HttpStream(args->context));
        if (!request)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_NOT_JSON);
        }

        if (!(args->matrixArgs->config->flags & TELODENDRIA_REGISTRATION))
        {
            HttpResponseStatus(args->context, HTTP_FORBIDDEN);
            response = MatrixErrorCreate(M_FORBIDDEN);
            goto finish;
        }

        val = HashMapGet(request, "username");
        if (val)
        {
            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                goto finish;
            }
            username = StrDuplicate(JsonValueAsString(val));

            if (!UserValidate(username, args->matrixArgs->config->serverName))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_USERNAME);
                goto finish;
            }

            if (UserExists(db, username))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_USER_IN_USE);
                goto finish;
            }
        }

        uiaFlows = ArrayCreate();
        ArrayAdd(uiaFlows, UiaDummyFlow());

        /* TODO: Add registration token flow */

        uiaResult = UiaComplete(uiaFlows, args->context,
            args->matrixArgs->db, request, &response);

        if (uiaResult < 0)
        {
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN);
            goto finish;
        }
        else if (!uiaResult)
        {
            /* UiaComplete() sets the response and status for us. */
            goto finish;
        }

        kind = HashMapGet(HttpRequestParams(args->context), "kind");

        /* We don't support guest accounts yet */
        if (kind && strcmp(kind, "user") != 0)
        {
            HttpResponseStatus(args->context, HTTP_FORBIDDEN);
            response = MatrixErrorCreate(M_INVALID_PARAM);
            goto finish;
        }


        val = HashMapGet(request, "password");
        if (!val)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_MISSING_PARAM);
            goto finish;
        }

        if (JsonValueType(val) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON);
            goto finish;
        }

        password = StrDuplicate(JsonValueAsString(val));

        val = HashMapGet(request, "device_id");
        if (val)
        {
            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                goto finish;
            }

            deviceId = StrDuplicate(JsonValueAsString(val));
        }

        val = HashMapGet(request, "inhibit_login");
        if (val)
        {
            if (JsonValueType(val) != JSON_BOOLEAN)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                goto finish;
            }

            inhibitLogin = JsonValueAsBoolean(val);
        }

        val = HashMapGet(request, "initial_device_display_name");
        if (val)
        {
            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                goto finish;
            }

            initialDeviceDisplayName = StrDuplicate(JsonValueAsString(val));
        }

        val = HashMapGet(request, "refresh_token");
        if (val)
        {
            if (JsonValueType(val) != JSON_BOOLEAN)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                goto finish;
            }

            refreshToken = JsonValueAsBoolean(val);
        }

        /* These values are already set */
        (void) refreshToken;
        (void) inhibitLogin;

        /* These may be NULL */
        (void) initialDeviceDisplayName;
        (void) deviceId;

        user = UserCreate(db, username, password);
        response = HashMapCreate();
        HashMapSet(response, "user_id", JsonValueString(StrConcat(4, "@",
                                                                  UserGetName(user), ":", args->matrixArgs->config->serverName)));
        HttpResponseStatus(args->context, HTTP_OK);
        if (!inhibitLogin)
        {
            UserLoginInfo *loginInfo = UserLogin(user, password, deviceId,
                              initialDeviceDisplayName, refreshToken);

            HashMapSet(response, "access_token",
                     JsonValueString(loginInfo->accessToken->string));
            HashMapSet(response, "device_id",
                   JsonValueString(loginInfo->accessToken->deviceId));

            if (refreshToken)
            {
                HashMapSet(response, "expires_in_ms",
                  JsonValueInteger(loginInfo->accessToken->lifetime));
                HashMapSet(response, "refresh_token",
                           JsonValueString(loginInfo->refreshToken));
            }

            /*
             * Don't need to free members; they're attached to the JSON response,
             * they will be freed after the response is sent.
             */
            Free(loginInfo->accessToken);
            Free(loginInfo);
        }

        Log(lc, LOG_INFO, "Registered user '%s'", UserGetName(user));

        UserUnlock(user);
finish:
        Free(username);
        Free(password);
        Free(deviceId);
        Free(initialDeviceDisplayName);
        JsonFree(request);
    }
    else
    {
        char *pathPart = MATRIX_PATH_POP(args->path);

        if (HttpRequestMethodGet(args->context) == HTTP_GET &&
            MATRIX_PATH_EQUALS(pathPart, "available"))
        {
            username = HashMapGet(
                        HttpRequestParams(args->context), "username");

            if (!username)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
            }
            else if (!UserValidate(username, args->matrixArgs->config->serverName))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_USERNAME);
            }
            else if (!UserExists(db, username))
            {
                response = HashMapCreate();
                HashMapSet(response, "available", JsonValueBoolean(1));
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_USER_IN_USE);
            }
        }
        else if (HttpRequestMethodGet(args->context) == HTTP_POST &&
                 (MATRIX_PATH_EQUALS(pathPart, "email") ||
                  MATRIX_PATH_EQUALS(pathPart, "msisdn")))
        {
            Free(pathPart);
            pathPart = MATRIX_PATH_POP(args->path);
            if (!MATRIX_PATH_EQUALS(pathPart, "requestToken"))
            {
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_UNRECOGNIZED);
            }
            else
            {
                /* TODO: Validate request body and potentially return
                 * M_BAD_JSON */
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_THREEPID_DENIED);
            }
        }
        else
        {
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_UNRECOGNIZED);
        }

        Free(pathPart);
    }

    return response;
}
