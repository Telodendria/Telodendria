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

ROUTE_IMPL(RouteLogin, path, argp)
{
    RouteArgs *args = argp;
    HashMap *request = NULL;
    HashMap *response = NULL;
    Array *enabledFlows;
    HashMap *pwdFlow;

    JsonValue *val;

    HashMap *identifier;

    char *deviceId = NULL;
    char *initialDeviceDisplayName = NULL;
    int refreshToken = 0;

    char *password;
    char *type;
    UserId *userId = NULL;

    Db *db = args->matrixArgs->db;

    User *user;
    UserLoginInfo *loginInfo;
    char *fullUsername;

    (void) path;

    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_GET:
            response = HashMapCreate();
            enabledFlows = ArrayCreate();
            pwdFlow = HashMapCreate();

            HashMapSet(pwdFlow, "type", JsonValueString("m.login.password"));
            ArrayAdd(enabledFlows, JsonValueObject(pwdFlow));
            HashMapSet(response, "flows", JsonValueArray(enabledFlows));
            break;
        case HTTP_POST:
            request = JsonDecode(HttpServerStream(args->context));
            if (!request)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_NOT_JSON);
                break;
            }

            val = HashMapGet(request, "type");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                break;
            }

            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            type = JsonValueAsString(val);
            if (strcmp(type, "m.login.password") != 0)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED);
                break;
            }

            val = HashMapGet(request, "identifier");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                break;
            }

            if (JsonValueType(val) != JSON_OBJECT)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            identifier = JsonValueAsObject(val);

            val = HashMapGet(identifier, "type");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                break;
            }

            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            type = JsonValueAsString(val);
            if (strcmp(type, "m.id.user") != 0)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED);
                break;
            }

            val = HashMapGet(identifier, "user");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                break;
            }

            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            userId = UserIdParse(JsonValueAsString(val),
                                 args->matrixArgs->config->serverName);
            if (!userId)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            if (strcmp(userId->server, args->matrixArgs->config->serverName) != 0
                || !UserExists(db, userId->localpart))
            {
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN);
                break;
            }

            val = HashMapGet(request, "device_id");
            if (val)
            {
                if (JsonValueType(val) != JSON_STRING)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_BAD_JSON);
                    break;
                }

                deviceId = JsonValueAsString(val);
            }

            val = HashMapGet(request, "initial_device_display_name");
            if (val)
            {
                if (JsonValueType(val) != JSON_STRING)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_BAD_JSON);
                    break;
                }

                initialDeviceDisplayName = JsonValueAsString(val);
            }

            val = HashMapGet(request, "password");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
                break;
            }

            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON);
                break;
            }

            password = JsonValueAsString(val);

            val = HashMapGet(request, "refresh_token");
            if (val)
            {
                if (JsonValueType(val) != JSON_BOOLEAN)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_BAD_JSON);
                    break;
                }

                refreshToken = JsonValueAsBoolean(val);
            }

            user = UserLock(db, userId->localpart);

            if (!user)
            {
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN);
                break;
            }

            if (UserDeactivated(user))
            {
                UserUnlock(user);

                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_USER_DEACTIVATED);
                break;
            }

            loginInfo = UserLogin(user, password, deviceId,
                              initialDeviceDisplayName, refreshToken);

            if (!loginInfo)
            {
                UserUnlock(user);

                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN);
                break;
            }

            response = HashMapCreate();

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

            fullUsername = StrConcat(4, "@", UserGetName(user), ":",
                                args->matrixArgs->config->serverName);
            HashMapSet(response, "user_id", JsonValueString(fullUsername));
            Free(fullUsername);

            HashMapSet(response, "well_known",
                       JsonValueObject(
              MatrixClientWellKnown(args->matrixArgs->config->baseUrl,
                          args->matrixArgs->config->identityServer)));

            UserAccessTokenFree(loginInfo->accessToken);
            Free(loginInfo->refreshToken);
            Free(loginInfo);

            UserUnlock(user);

            break;
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED);
            break;
    }

    UserIdFree(userId);
    JsonFree(request);
    return response;
}
