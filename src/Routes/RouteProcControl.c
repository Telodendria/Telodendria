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

#include <Cytoplasm/Int64.h>
#include <User.h>
#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Str.h>

#include <string.h>
#include <signal.h>

ROUTE_IMPL(RouteProcControl, path, argp)
{
    RouteArgs *args = argp;
    char *op = ArrayGet(path, 0);
    HashMap *response;
    char *token;
    char *msg;
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
        response = MatrixErrorCreate(M_UNKNOWN_TOKEN, NULL);
        goto finish;
    }

    if (!(UserGetPrivileges(user) & USER_PROC_CONTROL))
    {
        msg = "User doesn't have PROC_CONTROL privilege.";
        HttpResponseStatus(args->context, HTTP_FORBIDDEN);
        response = MatrixErrorCreate(M_FORBIDDEN, msg);
        goto finish;
    }

    msg = "Unknown operation.";
    switch (HttpRequestMethodGet(args->context))
    {
        case HTTP_POST:
            if (StrEquals(op, "restart"))
            {
                raise(SIGUSR1);
            }
            else if (StrEquals(op, "shutdown"))
            {
                raise(SIGINT);
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
                goto finish;
            }
            break;
        case HTTP_GET:
            if (StrEquals(op, "stats"))
            {
                size_t allocated = MemoryAllocated();
                Int64 a;

                response = HashMapCreate();

                if (sizeof(size_t) == sizeof(Int64))
                {
                    UInt32 high = (UInt32) (allocated >> 32);
                    UInt32 low = (UInt32) (allocated);

                    a = Int64Create(high, low);
                }
                else
                {
                    a = Int64Create(0, allocated);
                }

                HashMapSet(response, "version", JsonValueString(TELODENDRIA_VERSION));
                HashMapSet(response, "memory_allocated", JsonValueInteger(a));

                goto finish;
            }
            else
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
                goto finish;
            }
        default:
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNRECOGNIZED, msg);
            goto finish;
            break;
    }

    response = HashMapCreate();

finish:
    UserUnlock(user);
    return response;
}
