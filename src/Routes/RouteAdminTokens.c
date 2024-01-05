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

#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Memory.h>

#include <RegToken.h>
#include <User.h>

ROUTE_IMPL(RouteAdminTokens, path, argp)
{
    RouteArgs *args = argp;
    HashMap *request = NULL;
    HashMap *response = NULL;

    char *token;
    char *msg;

    Db *db = args->matrixArgs->db;

    User *user = NULL;

    HttpRequestMethod method = HttpRequestMethodGet(args->context);

    Array *tokensarray;
    Array *tokens;

    RegTokenInfo *info;

    RegTokenInfo *req;

    size_t i;

    if (method != HTTP_GET && method != HTTP_POST && method != HTTP_DELETE)
    {
        msg = "Route only supports GET, POST, and DELETE";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }
    

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

    if (!(UserGetPrivileges(user) & USER_ISSUE_TOKENS))
    {
        msg = "User doesn't have the ISSUE_TOKENS privilege.";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
        goto finish;
    }

    switch (method)
    {
        case HTTP_GET:
            if (ArraySize(path) == 0)
            {
                tokensarray = ArrayCreate();
                
                /* Get all registration tokens */
                tokens = DbList(db, 2, "tokens", "registration");

                response = HashMapCreate();

                for (i = 0; i < ArraySize(tokens); i++)
                {
                    char *tokenname = ArrayGet(tokens, i);
                    HashMap *jsoninfo;

                    info = RegTokenGetInfo(db, tokenname);
                    jsoninfo = RegTokenInfoToJson(info);
                    
                    RegTokenClose(info);
                    RegTokenFree(info);

                    ArrayAdd(tokensarray, JsonValueObject(jsoninfo));
                }

                JsonSet(response, JsonValueArray(tokensarray), 1, "tokens");

                DbListFree(tokens);
                break;
            }
            
            info = RegTokenGetInfo(db, ArrayGet(path, 0));
            if (!info)
            {
                msg = "Token doesn't exist.";
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_PARAM, msg);
                goto finish;
            }

            response = RegTokenInfoToJson(info);

            RegTokenClose(info);
            RegTokenFree(info);
            break;
        case HTTP_POST:
            request = JsonDecode(HttpServerStream(args->context));
            if (!request)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_NOT_JSON, NULL);
                goto finish;
            }

            req = Malloc(sizeof(RegTokenInfo));
            memset(req, 0, sizeof(RegTokenInfo));

            if (!RegTokenInfoFromJson(request, req, &msg))
            {
                RegTokenInfoFree(req);
                Free(req);

                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_BAD_JSON, msg);
                goto finish;
                
            }

            if (!req->name)
            {
                req->name = StrRandom(16);
            }
            
            /* Create the actual token that will be stored. */
            info = RegTokenCreate(db, req->name, UserGetName(user),
                                  req->expires_on, req->uses,
                                  UserDecodePrivileges(req->grants));
            if (!info)
            {
                RegTokenClose(info);
                RegTokenFree(info);
                RegTokenInfoFree(req);
                Free(req);
                
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                msg = "Cannot create token.";
                response = MatrixErrorCreate(M_INVALID_PARAM, msg);
                goto finish;
            }

            response = RegTokenInfoToJson(info);

            RegTokenClose(info);
            RegTokenFree(info);
            RegTokenInfoFree(req);
            Free(req);
            break;
        case HTTP_DELETE:
            if (ArraySize(path) == 0)
            {
                msg = "No registration token given to DELETE /tokens/[token].";
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_INVALID_PARAM, msg);
                goto finish;
            }
            info = RegTokenGetInfo(db, ArrayGet(path, 0));
            RegTokenDelete(info);

            response = HashMapCreate();
            break;
        default:
            /* Should not be possible. */
            break;
    }
finish: 
    UserUnlock(user);
    JsonFree(request);
    return response;
}
