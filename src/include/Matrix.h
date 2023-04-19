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
#ifndef TELODENDRIA_MATRIX_H
#define TELODENDRIA_MATRIX_H

#include <HttpServer.h>
#include <HttpRouter.h>
#include <Log.h>
#include <HashMap.h>

#include <Config.h>
#include <Db.h>

typedef enum MatrixError
{
    M_FORBIDDEN,
    M_UNKNOWN_TOKEN,
    M_MISSING_TOKEN,
    M_BAD_JSON,
    M_NOT_JSON,
    M_NOT_FOUND,
    M_LIMIT_EXCEEDED,
    M_UNKNOWN,
    M_UNRECOGNIZED,
    M_UNAUTHORIZED,
    M_USER_DEACTIVATED,
    M_USER_IN_USE,
    M_INVALID_USERNAME,
    M_ROOM_IN_USE,
    M_INVALID_ROOM_STATE,
    M_THREEPID_IN_USE,
    M_THREEPID_NOT_FOUND,
    M_THREEPID_AUTH_FAILED,
    M_THREEPID_DENIED,
    M_SERVER_NOT_TRUSTED,
    M_UNSUPPORTED_ROOM_VERSION,
    M_INCOMPATIBLE_ROOM_VERSION,
    M_BAD_STATE,
    M_GUEST_ACCESS_FORBIDDEN,
    M_CAPTCHA_NEEDED,
    M_CAPTCHA_INVALID,
    M_MISSING_PARAM,
    M_INVALID_PARAM,
    M_TOO_LARGE,
    M_EXCLUSIVE,
    M_RESOURCE_LIMIT_EXCEEDED,
    M_CANNOT_LEAVE_SERVER_NOTICE_ROOM
} MatrixError;

typedef struct MatrixHttpHandlerArgs
{
    Db *db;
    HttpRouter *router;
} MatrixHttpHandlerArgs;

extern void
 MatrixHttpHandler(HttpServerContext *, void *);

extern HashMap *
 MatrixErrorCreate(MatrixError);

extern HashMap *
 MatrixGetAccessToken(HttpServerContext *, char **);

extern HashMap *
 MatrixRateLimit(HttpServerContext *, Db *);

extern HashMap *
 MatrixClientWellKnown(char *, char *);

#endif
