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

#include <Schema/LoginRequest.h>

#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Memory.h>
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

    LoginRequest loginRequest;
    LoginRequestUserIdentifier userIdentifier;

    UserId *userId = NULL;

    Db *db = args->matrixArgs->db;

    User *user;
    UserLoginInfo *loginInfo;
    char *fullUsername;

    char *type;
    char *initialDeviceDisplayName;
    char *deviceId;
    char *password;
    int refreshToken;

    char *msg;

    Config *config = ConfigLock(db);

    if (!config)
    {
        Log(LOG_ERR, "Login endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, NULL);
    }

    (void) path;

    memset(&loginRequest, 0, sizeof(LoginRequest));
    memset(&userIdentifier, 0, sizeof(LoginRequestUserIdentifier));

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
                response = MatrixErrorCreate(M_NOT_JSON, NULL);
                break;
            }

            if (!LoginRequestFromJson(request, &loginRequest, &msg))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, msg);
                break;
            }

            if (loginRequest.type != REQUEST_TYPE_PASSWORD)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
                break;
            }

            identifier = loginRequest.identifier;

            val = HashMapGet(identifier, "type");
            if (!val)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM, NULL);
                break;
            }

            if (JsonValueType(val) != JSON_STRING)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, NULL);
                break;
            }

            type = JsonValueAsString(val);
            if (!StrEquals(type, "m.id.user"))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
                break;
            }
            if (!LoginRequestUserIdentifierFromJson(identifier, 
                                        &userIdentifier, &msg))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, msg);
                break;
            }


            userId = UserIdParse(userIdentifier.user, config->serverName);
            if (!userId)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, NULL);
                break;
            }

            if (!StrEquals(userId->server, config->serverName)
                || !UserExists(db, userId->localpart))
            {
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN, NULL);
                break;
            }
            
            deviceId = loginRequest.device_id;

            initialDeviceDisplayName =loginRequest.initial_device_display_name;
            password = loginRequest.password;
            refreshToken = loginRequest.refresh_token;

            user = UserLock(db, userId->localpart);

            if (!user)
            {
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN, NULL);
                break;
            }

            if (UserDeactivated(user))
            {
                UserUnlock(user);

                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_USER_DEACTIVATED, NULL);
                break;
            }

            loginInfo = UserLogin(user, password, deviceId,
                              initialDeviceDisplayName, refreshToken);

            if (!loginInfo)
            {
                UserUnlock(user);

                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_FORBIDDEN, NULL);
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
                                     config->serverName);
            HashMapSet(response, "user_id", JsonValueString(fullUsername));
            Free(fullUsername);

            HashMapSet(response, "well_known",
                       JsonValueObject(
                                       MatrixClientWellKnown(config->baseUrl, config->identityServer)));

            UserAccessTokenFree(loginInfo->accessToken);
            Free(loginInfo->refreshToken);
            Free(loginInfo);

            UserUnlock(user);

            break;
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
            break;
    }

    UserIdFree(userId);
    JsonFree(request);
    ConfigUnlock(config);

    LoginRequestFree(&loginRequest);
    LoginRequestUserIdentifierFree(&userIdentifier);

    return response;
}
