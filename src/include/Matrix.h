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

/***
 * @Nm Matrix
 * @Nd Functions for writing Matrix API Endpoints.
 * @Dd March 6 2023
 * @Xr HttpServer Log Config Db
 *
 * .Nm
 * provides some helper functions that bind to the HttpServer API and
 * add basic Matrix functionality, turning an HTTP server into a
 * Matrix homeserver.
 */

#include <HttpServer.h>
#include <HttpRouter.h>
#include <Log.h>
#include <HashMap.h>

#include <Config.h>
#include <Db.h>

/**
 * The valid errors that can be used with
 * .Fn MatrixErrorCreate .
 * These values exactly follow the errors defined in the Matrix
 * specification.
 */
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

/**
 * The arguments that should be passed through the void pointer to the
 * .Fn MatrixHttpHandler
 * function. This structure should be populated once, and then never
 * modified again for the duration of the HTTP server.
 */
typedef struct MatrixHttpHandlerArgs
{
    Db *db;
    HttpRouter *router;
} MatrixHttpHandlerArgs;

/**
 * The HTTP handler function that handles all Matrix homeserver
 * functionality. It should be passed into
 * .Fn HttpServerCreate ,
 * and it expects that a pointer to a MatrixHttpHandlerArgs
 * will be provided, because that is what the void pointer is
 * cast to.
 */
extern void MatrixHttpHandler(HttpServerContext *, void *);

/**
 * A convenience function that constructs an error payload, including
 * the error code and message, given a MatrixError and an optional 
 * message.
 */
extern HashMap * MatrixErrorCreate(MatrixError, char *);

/**
 * Read the request headers and parameters, and attempt to obtain an
 * access token from them. The Matrix specification says that an access
 * token can either be provided via the Authorization header, or in a
 * .Sy GET
 * parameter. This function checks both, and stores the access token it
 * finds in the passed character pointer.
 * .Pp
 * The specification does not say whether the header or parameter
 * should be preferred if both are provided. This function prefers the
 * header.
 * .Pp
 * If this function returns a non-NULL value, then the return value
 * should be immediately passed along to the client and no further
 * logic should be performed.
 */
extern HashMap * MatrixGetAccessToken(HttpServerContext *, char **);

/**
 * Determine whether or not the request should be rate limited. It is
 * expected that this function will be called before most, if not all
 * of the caller's logic.
 * .Pp
 * If this function returns a non-NULL value, then the return value
 * should be immediately passed along to the client and no further
 * logic should be performed.
 */
extern HashMap * MatrixRateLimit(HttpServerContext *, Db *);

/**
 * Build a ``well-known'' JSON object, which contains information
 * about the homeserver base URL and identity server, both of which
 * should be provided by the caller in that order. This object can be
 * sent to a client as-is, or it can be added as a value nested inside
 * of a more complex response. Both occur in the Matrix specification.
 */
extern HashMap * MatrixClientWellKnown(char *, char *);

#endif
