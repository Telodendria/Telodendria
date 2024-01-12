/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Str.h>

#include <Config.h>
#include <Parser.h>
#include <User.h>

ROUTE_IMPL(RouteAliasDirectory, path, argp)
{
    RouteArgs *args = argp;
    char *alias = ArrayGet(path, 0);

    HashMap *request = NULL;
    HashMap *response;

    Db *db = args->matrixArgs->db;
    DbRef *ref = NULL;
    HashMap *aliases;
    HashMap *idObject;
    JsonValue *val;
    Array *arr;

    char *token;
    char *msg;
    User *user = NULL;

    CommonID aliasID;
    Config config;

    aliasID.sigil = '\0';
    aliasID.local = NULL;
    aliasID.server.hostname = NULL;
    aliasID.server.port = NULL;

    ConfigLock(db, &config);

    if (!ParseCommonID(alias, &aliasID) || aliasID.sigil != '#')
    {
        msg = "Invalid room alias.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_INVALID_PARAM, msg);
        goto finish;
    }

    ref = DbLock(db, 1, "aliases");
    if (!ref && !(ref = DbCreate(db, 1, "aliases")))
    {
        msg = "Unable to access alias database.",
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN, msg);
        goto finish;
    }

    aliases = DbJson(ref);

    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_GET:
            val = JsonGet(aliases, 2, "alias", alias);
            if (val)
            {
                response = HashMapCreate();
                HashMapSet(response, "room_id", JsonValueDuplicate(HashMapGet(JsonValueAsObject(val), "id")));
                HashMapSet(response, "servers", JsonValueDuplicate(JsonGet(aliases, 3, "alias", alias, "servers")));
            }
            else
            {
                msg = "There is no mapped room ID for this room alias.";
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_NOT_FOUND, msg);
            }
            break;
        case HTTP_PUT:
        case HTTP_DELETE:
            response = MatrixGetAccessToken(args->context, &token);
            if (response)
            {
                goto finish;
            }

            user = UserAuthenticate(db, token);
            if (!user)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
                goto finish;
            }

            if (HttpRequestMethodGet(args->context) == HTTP_PUT)
            {
                HashMap *newAlias;
                char *id;
                char *serverPart;

                serverPart = ParserRecomposeServerPart(aliasID.server);
                if (!StrEquals(serverPart, config.serverName))
                {
                    msg = "Invalid server name.";
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_INVALID_PARAM, msg);

                    Free(serverPart);
                    goto finish;
                }

                Free(serverPart);

                if (JsonGet(aliases, 2, "alias", alias))
                {
                    HttpResponseStatus(args->context, HTTP_CONFLICT);
                    response = MatrixErrorCreate(M_UNKNOWN, "Room alias already exists.");
                    goto finish;
                }

                request = JsonDecode(HttpServerStream(args->context));
                if (!request)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_NOT_JSON, NULL);
                    goto finish;
                }

                id = JsonValueAsString(HashMapGet(request, "room_id"));
                if (!id)
                {
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_BAD_JSON, "Missing or invalid room_id.");
                    goto finish;
                }
                
                if (!ValidCommonID(id, '!'))
                {
                    msg = "Invalid room ID.";
                    HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                    response = MatrixErrorCreate(M_INVALID_PARAM, msg);
                    goto finish;
                }

                newAlias = HashMapCreate();
                HashMapSet(newAlias, "createdBy", JsonValueString(UserGetName(user)));
                HashMapSet(newAlias, "id", JsonValueString(id));
                HashMapSet(newAlias, "servers", JsonValueArray(ArrayCreate()));

                JsonSet(aliases, JsonValueObject(newAlias), 2, "alias", alias);

                if (!(idObject = JsonValueAsObject(JsonGet(aliases, 2, "id", id))))
                {
                    arr = ArrayCreate();
                    idObject = HashMapCreate();
                    HashMapSet(idObject, "aliases", JsonValueArray(arr));
                    JsonSet(aliases, JsonValueObject(idObject), 2, "id", id);
                }
                val = HashMapGet(idObject, "aliases");
                arr = JsonValueAsArray(val);
                ArrayAdd(arr, JsonValueString(alias));

            }
            else
            {
                HashMap *roomAlias;
                char *id;

                if (!(val = JsonGet(aliases, 2, "alias", alias)))
                {
                    HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                    response = MatrixErrorCreate(M_NOT_FOUND, "Room alias not found.");
                    goto finish;
                }
                roomAlias = JsonValueAsObject(val);
                id = StrDuplicate(JsonValueAsString(HashMapGet(roomAlias, "id")));

                if (!(UserGetPrivileges(user) & USER_ALIAS) && !StrEquals(UserGetName(user), JsonValueAsString(JsonGet(roomAlias, 1, "createdBy"))))
                {
                    HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
                    response = MatrixErrorCreate(M_UNAUTHORIZED, NULL);
                    Free(id);
                    goto finish;
                }

                JsonValueFree(HashMapDelete(JsonValueAsObject(HashMapGet(aliases, "alias")), alias));

                idObject = JsonValueAsObject(JsonGet(aliases, 2, "id", id));
                if (idObject)
                {
                    size_t i;
                    val = HashMapGet(idObject, "aliases");
                    arr = JsonValueAsArray(val);
                    for (i = 0; i < ArraySize(arr); i++)
                    {
                        if (StrEquals(JsonValueAsString(ArrayGet(arr, i)), alias))
                        {
                            JsonValueFree(ArrayDelete(arr, i));
                            break;
                        }
                    }
                }
                Free(id);
            }
            response = HashMapCreate();

            break;
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, "Unknown request method.");
            goto finish;
    }

finish:
    CommonIDFree(aliasID);
    ConfigUnlock(&config);
    UserUnlock(user);
    DbUnlock(db, ref);
    JsonFree(request);
    return response;
}
