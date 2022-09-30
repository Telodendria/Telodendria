/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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
#include <stdlib.h>

#include <Json.h>
#include <HashMap.h>

ROUTE(RouteWellKnown)
{
    HashMap *response = NULL;
    char *pathPart = MATRIX_PATH_POP(path);

    if (!MATRIX_PATH_EQUALS(pathPart, "matrix") || MATRIX_PATH_PARTS(path) != 1)
    {
        free(pathPart);
        HttpResponseStatus(context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }

    free(pathPart);
    pathPart = MATRIX_PATH_POP(path);

    if (MATRIX_PATH_EQUALS(pathPart, "client"))
    {
        HashMap *homeserver = HashMapCreate();

        free(pathPart);

        response = HashMapCreate();

        HashMapSet(homeserver, "base_url", JsonValueString(UtilStringDuplicate(args->config->baseUrl)));
        HashMapSet(response, "m.homeserver", JsonValueObject(homeserver));

        if (args->config->identityServer)
        {
            HashMap *identityServer = HashMapCreate();

            HashMapSet(identityServer, "base_url", JsonValueString(UtilStringDuplicate(args->config->identityServer)));
            HashMapSet(response, "m.identity_server", identityServer);
        }

        return response;
    }
    else
    {
        free(pathPart);
        HttpResponseStatus(context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }
}
