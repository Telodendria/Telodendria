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

ROUTE_IMPL(RouteProcControl, path, argp)
{
    RouteArgs *args = argp;
    char *op = ArrayGet(path, 0);
    HashMap *response;
    char *token;
    User *user = NULL;

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

    if (!(UserGetPrivileges(user) & USER_PROC_CONTROL))
    {
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN);
        goto finish;
    }

    if (strcmp(op, "restart") == 0)
    {
        Restart();
    }
    else if (strcmp(op, "shutdown") == 0)
    {
        Shutdown();
    }
    else if (strcmp(op, "stats") == 0)
    {
        response = HashMapCreate();

        HashMapSet(response, "version", JsonValueString(TELODENDRIA_VERSION));
        HashMapSet(response, "memory_allocated", JsonValueInteger(MemoryAllocated()));
        HashMapSet(response, "uptime", JsonValueInteger(Uptime()));

        goto finish;
    }
    else
    {
        /* Should be impossible */
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        response = MatrixErrorCreate(M_UNKNOWN);
        goto finish;
    }

    response = HashMapCreate();

finish:
    UserUnlock(user);
    return response;
}
