/*
 * Copyright (C) 2022-2025 Jordan Bancino <@jordan:synapse.telodendria.org>
 * with other valuable contributors. See CONTRIBUTORS.txt for the full list.
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


#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Str.h>

#include <User.h>
#include <string.h>

#include <Schema/Filter.h>

static char *
GetServerName(Db * db)
{
    char *name;

    Config config;

    ConfigLock(db, &config);
    if (!config.ok)
    {
        return NULL;
    }

    name = StrDuplicate(config.serverName);

    ConfigUnlock(&config);

    return name;
}

ROUTE_IMPL(RouteFilter, path, argp)
{
    RouteArgs *args = argp;
    Db *db = args->matrixArgs->db;

    HashMap *request = NULL;
    HashMap *response = NULL;

    User *user = NULL;
    CommonID *id = NULL;
    char *token = NULL;

    char *serverName = NULL;

    char *userParam = ArrayGet(path, 0);

    char *msg;

    if (!userParam)
    {
        /* Should be impossible */
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, NULL);
    }

    serverName = GetServerName(db);
    if (!serverName)
    {
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, NULL);
        goto finish;
    }

    id = UserIdParse(userParam, serverName);
    if (!id)
    {
        msg = "Invalid user ID.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_INVALID_PARAM, msg);
        goto finish;
    }

    if (!ParserServerNameEquals(id->server, serverName))
    {
        msg = "Cannot use /filter for non-local users.";
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_UNAUTHORIZED, msg);
        goto finish;
    }

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        goto finish;
    }

    user = UserAuthenticate(db, token);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    if (!StrEquals(id->local, UserGetName(user)))
    {
        msg = "Unauthorized to use /filter.";
        HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
        response = MatrixErrorCreate(M_INVALID_PARAM, msg);
        goto finish;
    }

    if (ArraySize(path) == 2 && HttpRequestMethodGet(args->context) == HTTP_GET)
    {
        DbRef *ref = DbLock(db, 3, "filters", UserGetName(user), ArrayGet(path, 1));

        if (!ref)
        {
            msg = "The filter for this user was not found.";
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_NOT_FOUND, msg);
            goto finish;
        }

        response = JsonDuplicate(DbJson(ref));
        DbUnlock(db, ref);
    }
    else if (ArraySize(path) == 1 && HttpRequestMethodGet(args->context) == HTTP_POST)
    {
        DbRef *ref;
        char *filterId;

        Filter filter = {0};
        char *parseErr;
        HashMap *filterJson;

        request = JsonDecode(HttpServerStream(args->context));
        if (!request)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_NOT_JSON, NULL);
            goto finish;
        }

        if (!FilterFromJson(request, &filter, &parseErr))
        {
            response = MatrixErrorCreate(M_BAD_JSON, parseErr);
            goto finish;
        }


        filterId = StrRandom(12);
        if (!filterId)
        {
            msg = "Couldn't generate random filter ID; this is unintended.";
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, msg);
            goto finish;
        }

        ref = DbCreate(db, 3, "filters", UserGetName(user), filterId);
        if (!ref)
        {
            Free(filterId);
            msg = "Couldn't write filter to the database, this is unintended.";
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, msg);
            goto finish;
        }

        filterJson = FilterToJson(&filter);
        DbJsonSet(ref, filterJson);
        DbUnlock(db, ref);

        Free(filterJson);

        response = HashMapCreate();
        HashMapSet(response, "filter_id", JsonValueString(filterId));
        Free(filterId);

        FilterFree(&filter);
    }
    else
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNRECOGNIZED, NULL);
    }

finish:
    Free(serverName);
    UserIdFree(id);
    UserUnlock(user);
    JsonFree(request);
    return response;
}
