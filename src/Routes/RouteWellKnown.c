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
#include <Util.h>

ROUTE_IMPL(RouteWellKnown, args)
{
    HashMap *response = NULL;
    char *pathPart = MATRIX_PATH_POP(args->path);

    if (!MATRIX_PATH_EQUALS(pathPart, "matrix") || MATRIX_PATH_PARTS(args->path) != 1)
    {
        Free(pathPart);
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }

    Free(pathPart);
    pathPart = MATRIX_PATH_POP(args->path);

    if (MATRIX_PATH_EQUALS(pathPart, "client"))
    {
        HashMap *homeserver = HashMapCreate();

        Free(pathPart);

        response = HashMapCreate();

        HashMapSet(homeserver, "base_url", JsonValueString(UtilStringDuplicate(args->matrixArgs->config->baseUrl)));
        HashMapSet(response, "m.homeserver", JsonValueObject(homeserver));

        if (args->matrixArgs->config->identityServer)
        {
            HashMap *identityServer = HashMapCreate();

            HashMapSet(identityServer, "base_url", JsonValueString(UtilStringDuplicate(args->matrixArgs->config->identityServer)));
            HashMapSet(response, "m.identity_server", identityServer);
        }

        return response;
    }
    else
    {
        Free(pathPart);
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND);
    }
}
