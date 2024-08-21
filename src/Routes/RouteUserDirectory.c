/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without dirRequest.limitation the rights to use, copy, modify, merge,
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

#include <Cytoplasm/Array.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Db.h>

#include <Schema/UserDirectoryRequest.h>

#include <User.h>

ROUTE_IMPL(RouteUserDirectory, path, argp)
{
    RouteArgs *args = argp;
    HashMap *response = NULL;
    HashMap *request = NULL;

    Array *users = NULL;
    Array *results = NULL;

    Db *db = args->matrixArgs->db;

    Config config = { .ok = 0 };

    User *user = NULL;

    char *token = NULL;
    char *requesterName = NULL;
    char *msg = NULL;

    UserDirectoryRequest dirRequest;

    size_t i, included;

    (void) path;

    dirRequest.search_term = NULL;
    dirRequest.limit = 10;


    if (HttpRequestMethodGet(args->context) != HTTP_POST)
    {
        msg = "Request supports only POST.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
        goto finish;
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_NOT_JSON, NULL);
        goto finish;
    }
    if (!UserDirectoryRequestFromJson(request, &dirRequest, &msg))
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;
    }
    if (!dirRequest.search_term)
    {
        msg = "Field 'search_term' not set.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_BAD_JSON, msg);
        goto finish;

    }

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        return response;
    }

    /* TODO: Actually use information related to the user. */
    user = UserAuthenticate(db, token);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }
    requesterName = UserGetName(user);

    response = HashMapCreate();
    results = ArrayCreate();

    /* TODO: Check for users matching search term and users outside our 
     * local server. */
    users = DbList(db, 1, "users");

    ConfigLock(db, &config);
    if (!config.ok)
    {
        Log(LOG_ERR, "Directory endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, config.err);

        goto finish;
    }

#define IncludedLtLimit ((int64_t) included < dirRequest.limit)
    for (i = 0, included = 0; i < ArraySize(users) && IncludedLtLimit; i++)
#undef IncludedLtLimit
    {
        HashMap *obj;
        User *currentUser;
        char *name = ArrayGet(users, i);
        char *displayName;
        char *lowerDisplayName;
        char *avatarUrl;

        if (!StrEquals(name, requesterName))
        {
            currentUser = UserLock(db, name);
        }
        else
        {
            currentUser = user;
        }

        displayName = UserGetProfile(currentUser, "displayname");
        lowerDisplayName = StrLower(displayName);
        avatarUrl = UserGetProfile(currentUser, "avatar_url");

        /* Check for the user ID and display name. */
        if (strstr(name, dirRequest.search_term) ||
            (lowerDisplayName && 
             strstr(lowerDisplayName, dirRequest.search_term)))
        {
            included++;

            obj = HashMapCreate();
            if (displayName)
            {
                JsonSet(obj, JsonValueString(displayName), 1, "display_name");
            }
            if (avatarUrl)
            {
                JsonSet(obj, JsonValueString(displayName), 1, "avatar_url");
            }
            if (name)
            {
                char *uID = StrConcat(4, "@", name, ":", config.serverName);
                JsonSet(obj, JsonValueString(uID), 1, "user_id");
                Free(uID);
            }
            ArrayAdd(results, JsonValueObject(obj));
        }
        if (lowerDisplayName)
        {
            Free(lowerDisplayName);
        }
        if (!StrEquals(name, requesterName))
        {
            UserUnlock(currentUser);
        }
    }
    JsonSet(response, JsonValueArray(results), 1, "results");
    JsonSet(response, 
        JsonValueBoolean((int64_t) included == dirRequest.limit), 
        1, "limited"
    );

finish:
    UserUnlock(user);
    JsonFree(request);
    DbListFree(users);
    ConfigUnlock(&config);
    UserDirectoryRequestFree(&dirRequest);
    return response;
}
