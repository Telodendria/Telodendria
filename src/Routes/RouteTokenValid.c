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

#include <string.h>

#include <RegToken.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Str.h>

ROUTE_IMPL(RouteTokenValid, path, argp)
{
    RouteArgs *args = argp;
    Db *db = args->matrixArgs->db;

    HashMap *response = NULL;
    HashMap *request = NULL;

    RegTokenInfo *info = NULL;

    char *tokenstr;
    char *msg;

    (void) path;

    if (HttpRequestMethodGet(args->context) != HTTP_GET)
    {
        msg = "This route only accepts GET.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }

    request = JsonDecode(HttpServerStream(args->context));
    if (!request)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_NOT_JSON, NULL);
    }

    tokenstr = JsonValueAsString(HashMapGet(request, "token"));
    if (!tokenstr || !RegTokenExists(db, tokenstr))
    {
        response = HashMapCreate();
        JsonFree(request);
        HashMapSet(response, "valid", JsonValueBoolean(0));

        return response;
    }

    info = RegTokenGetInfo(db, tokenstr);
    response = HashMapCreate();

    if (!RegTokenValid(info))
    {
        JsonFree(request);
        RegTokenClose(info);
        RegTokenFree(info);
        HashMapSet(response, "valid", JsonValueBoolean(0));

        return response;
    }

    RegTokenClose(info);
    RegTokenFree(info);
    HashMapSet(response, "valid", JsonValueBoolean(1));
    JsonFree(request);
    return response;
}
