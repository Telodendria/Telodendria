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
#include <Cytoplasm/Memory.h>

#include <Schema/Registration.h>

#include <User.h>
#include <Uia.h>
#include <RegToken.h>

static Array *
RouteRegisterRegFlow(void)
{
    Array *response = ArrayCreate();

    if (!response)
    {
        return NULL;
    }

    ArrayAdd(response, UiaStageBuild("m.login.registration_token", NULL));

    return response;
}

ROUTE_IMPL(RouteRegister, path, argp)
{
    RouteArgs *args = argp;
    HashMap *request = NULL;
    HashMap *response = NULL;

    RegistrationRequest regReq;

    char *kind;
    char *fullUsername;
    char *msg;
    char *username;

    Db *db = args->matrixArgs->db;
    User *user = NULL;
    Array *uiaFlows = NULL;
    int uiaResult;

    char *session;
    DbRef *sessionRef;

    Config *config = ConfigLock(db);

    regReq.username = NULL;
    regReq.password = NULL;
    regReq.device_id = NULL;
    regReq.initial_device_display_name = NULL;
    regReq.refresh_token = 0;
    regReq.inhibit_login = 0;




    if (!config)
    {
        msg = "Internal server error while locking configuration.";
        Log(LOG_ERR, "Registration endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, msg);
    }

    if (ArraySize(path) == 0)
    {
        if (HttpRequestMethodGet(args->context) != HTTP_POST)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
            goto end;
        }

        request = JsonDecode(HttpServerStream(args->context));
        if (!request)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_NOT_JSON, NULL);
            goto end;
        }
        if (!RegistrationRequestFromJson(request, &regReq, &msg))
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_NOT_JSON, msg);
            goto end;
        }

        if (regReq.username)
        {
            if (!UserValidate(regReq.username, config->serverName))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_USERNAME, NULL);
                goto finish;
            }

            if (UserExists(db, regReq.username))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_USER_IN_USE, NULL);
                goto finish;
            }
        }

        uiaFlows = ArrayCreate();
        ArrayAdd(uiaFlows, RouteRegisterRegFlow());

        if (config->flags & CONFIG_REGISTRATION)
        {
            ArrayAdd(uiaFlows, UiaDummyFlow());
        }

        uiaResult = UiaComplete(uiaFlows, args->context,
                                db, request, &response,
                                config);

        if (uiaResult < 0)
        {
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, NULL);
            goto finish;
        }
        else if (!uiaResult)
        {
            /* UiaComplete() sets the response and status for us. */
            goto finish;
        }

        kind = HashMapGet(HttpRequestParams(args->context), "kind");

        /* We don't support guest accounts yet */
        if (kind && !StrEquals(kind, "user"))
        {
            msg = "Guest accounts are currently not supported";
            HttpResponseStatus(args->context, HTTP_FORBIDDEN);
            response = MatrixErrorCreate(M_INVALID_PARAM, msg);
            goto finish;
        }

        if (!regReq.password)
        {
            msg = "'password' field is unset";
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_MISSING_PARAM, msg);
            goto finish;
        }

        /* All of the other fields are optional, we don't have to check 
         * them. */

        user = UserCreate(db, regReq.username, regReq.password);
        response = HashMapCreate();

        fullUsername = StrConcat(4, 
            "@", UserGetName(user), ":", config->serverName);
        HashMapSet(response, "user_id", JsonValueString(fullUsername));
        Free(fullUsername);

        HttpResponseStatus(args->context, HTTP_OK);
        if (!regReq.inhibit_login)
        {
            UserLoginInfo *loginInfo = UserLogin(user, regReq.password, 
                regReq.device_id, regReq.initial_device_display_name, 
                regReq.refresh_token);

            HashMapSet(response, "access_token",
                     JsonValueString(loginInfo->accessToken->string));
            HashMapSet(response, "device_id",
                   JsonValueString(loginInfo->accessToken->deviceId));

            if (regReq.refresh_token)
            {
                HashMapSet(response, "expires_in_ms",
                  JsonValueInteger(loginInfo->accessToken->lifetime));
                HashMapSet(response, "refresh_token",
                           JsonValueString(loginInfo->refreshToken));
            }

            UserAccessTokenFree(loginInfo->accessToken);
            Free(loginInfo->refreshToken);
            Free(loginInfo);
        }

        session = JsonValueAsString(JsonGet(request, 2, "auth", "session"));
        sessionRef = DbLock(db, 2, "user_interactive", session);
        if (sessionRef)
        {
            char *token = JsonValueAsString(HashMapGet(DbJson(sessionRef), "registration_token"));

            /* Grant the privileges specified by the given token */
            if (token)
            {
                RegTokenInfo *info = RegTokenGetInfo(db, token);

                if (info)
                {
                    UserSetPrivileges(user, UserDecodePrivileges(info->grants));
                    RegTokenClose(info);
                    RegTokenFree(info);
                }
            }
            DbUnlock(db, sessionRef);
        }
        else
        {
            Log(LOG_WARNING, "Unable to lock UIA session reference to check");
            Log(LOG_WARNING, "privileges for user registration.");
        }

        Log(LOG_INFO, "Registered user '%s'", UserGetName(user));

        UserUnlock(user);
finish:
        UiaFlowsFree(uiaFlows);
        RegistrationRequestFree(&regReq);
        JsonFree(request);
    }
    else
    {
        if (HttpRequestMethodGet(args->context) == HTTP_GET &&
            StrEquals(ArrayGet(path, 0), "available"))
        {
            username = HashMapGet(
                        HttpRequestParams(args->context), "username");

            if (!username)
            {
                msg = "'username' path parameter is not set.";
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM, msg);
            }
            else if (!UserValidate(username, config->serverName))
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_USERNAME, NULL);
            }
            else if (!UserExists(db, username))
            {
                response = HashMapCreate();
                HashMapSet(response, "available", JsonValueBoolean(1));
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_USER_IN_USE, NULL);
            }
        }
        else
        {
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
        }
    }

end:
    ConfigUnlock(config);
    return response;
}
