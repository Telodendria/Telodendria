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

#include <Memory.h>
#include <Json.h>
#include <HashMap.h>
#include <Str.h>

#include <User.h>

ROUTE_IMPL(RouteRefresh, args)
{
    HashMap *request;
    HashMap *response = NULL;

    JsonValue *val;
    char *refreshToken;

    char *oldAccessToken;
    UserAccessToken *newAccessToken;
    char *deviceId;

    Db *db = args->matrixArgs->db;
    LogConfig *lc = args->matrixArgs->lc;

    User *user = NULL;
    DbRef *rtRef = NULL;
    DbRef *oAtRef = NULL;

    if (MATRIX_PATH_PARTS(args->path) > 0)
    {
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }

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

    val = HashMapGet(request, "refresh_token");
    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON);
        goto finish;
    }

    refreshToken = JsonValueAsString(val);

    /* Get the refresh token object */
    rtRef = DbLock(db, 3, "tokens", "refresh", refreshToken);

    if (!rtRef)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN);
        goto finish;
    }

    /* Get the access token and device the refresh token refreshes */
    oldAccessToken = JsonValueAsString(HashMapGet(DbJson(rtRef), "refreshes"));
    oAtRef = DbLock(db, 3, "tokens", "access", oldAccessToken);

    if (!oAtRef)
    {
        Log(lc, LOG_ERR, "Refresh token '%s' points to an access token that doesn't exist.",
            refreshToken);
        Log(lc, LOG_WARNING, "This refresh token will be deleted.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN);

        DbUnlock(db, rtRef);
        DbDelete(db, 3, "tokens", "refresh", refreshToken);

        rtRef = NULL;

        goto finish;
    }

    /* Get the user associated with the access token and device */
    user = UserLock(db, JsonValueAsString(HashMapGet(DbJson(oAtRef), "user")));
    if (!user)
    {
        Log(lc, LOG_ERR, "Access token '%s' points to a user that doesn't exist.",
            oldAccessToken);
        Log(lc, LOG_WARNING, "This access token will be deleted.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN);

        DbUnlock(db, rtRef);
        DbDelete(db, 3, "tokens", "refresh", refreshToken);

        DbUnlock(db, oAtRef);
        DbDelete(db, 3, "tokens", "access", oldAccessToken);

        rtRef = NULL;
        oAtRef = NULL;

        goto finish;
    }

    /* Generate a new access token associated with the device and user. */
    deviceId = JsonValueAsString(HashMapGet(DbJson(oAtRef), "device"));
    newAccessToken = UserGenerateAccessToken(user, deviceId, 1);
    UserAccessTokenSave(db, newAccessToken);

    /* Replace old access token in User */
    JsonValueFree(JsonSet(UserGetDevices(user), JsonValueString(newAccessToken->string), 2, deviceId, "accessToken"));

    /* Delete old access token */
    DbUnlock(db, oAtRef);
    DbDelete(db, 3, "tokens", "access", oldAccessToken);

    /* Update the refresh token to point to the new access token */
    JsonValueFree(HashMapSet(DbJson(rtRef), "refreshes", JsonValueString(StrDuplicate(newAccessToken->string))));

    /* Return the new access token and expiration timestamp to the
     * client */
    response = HashMapCreate();
    HashMapSet(response, "access_token", JsonValueString(StrDuplicate(newAccessToken->string)));
    HashMapSet(response, "expires_in_ms", JsonValueInteger(newAccessToken->lifetime));

    Free(newAccessToken);

finish:
    JsonFree(request);
    DbUnlock(db, rtRef);
    UserUnlock(user);
    return response;
}