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

#include <User.h>
#include <Main.h>
#include <Memory.h>

#include <string.h>

ROUTE_IMPL(RouteConfig, path, argp)
{
    RouteArgs *args = argp;
    HashMap *response;
    char *token;

    User *user = NULL;
    Config *config = NULL;

    HashMap *request = NULL;
    Config *newConf;

    (void) path;

    response = MatrixGetAccessToken(args->context, &token);
    if (response)
    {
        goto finish;
    }

    user = UserAuthenticate(args->matrixArgs->db, token);
    if (!user)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN);
        goto finish;
    }

    if (!(UserGetPrivileges(user) & USER_CONFIG))
    {
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN);
        goto finish;
    }

    config = ConfigLock(args->matrixArgs->db);
    if (!config)
    {
        Log(LOG_ERR, "Config endpoint failed to lock configuration.");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN);
        goto finish;
    }

    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_GET:
            response = JsonDuplicate(DbJson(config->ref));
            break;
        case HTTP_POST:
            request = JsonDecode(HttpServerStream(args->context));
            if (!request)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_NOT_JSON);
                break;
            }

            newConf = ConfigParse(request);
            if (!newConf)
            {
                HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
                response = MatrixErrorCreate(M_UNKNOWN);
                break;
            }

            if (newConf->ok)
            {
                if (DbJsonSet(config->ref, request))
                {
                    response = HashMapCreate();
                    /*
                     * TODO: Apply configuration and set this only if a main
                     * component was reconfigured, such as the listeners.
                     */
                    HashMapSet(response, "restart_required", JsonValueBoolean(1));
                }
                else
                {
                    HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
                    response = MatrixErrorCreate(M_UNKNOWN);
                }
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                /* TODO: Attach newConf->err as message */
                response = MatrixErrorCreate(M_BAD_JSON);
            }

            ConfigFree(newConf);
            JsonFree(request);
            break;
        case HTTP_PUT:
            /* TODO: Support incremental changes to the config */
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED);
            break;
    }

finish:
    UserUnlock(user);
    ConfigUnlock(config);
    return response;
}
