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
#ifndef TELODENDRIA_ROUTES_H
#define TELODENDRIA_ROUTES_H

#include <string.h>

#include <HashMap.h>
#include <Array.h>
#include <HttpServer.h>
#include <Matrix.h>

#define MATRIX_PATH Array
#define MATRIX_PATH_CREATE() ArrayCreate()
#define MATRIX_PATH_APPEND(path, part) ArrayAdd(path, part)
#define MATRIX_PATH_FREE(path) ArrayFree(path)

#define MATRIX_PATH_POP(path) ArrayDelete(path, 0)
#define MATRIX_PATH_PARTS(path) ArraySize(path)

#define MATRIX_PATH_EQUALS(pathPart, str) \
	((pathPart != NULL) && (strcmp(pathPart, str) == 0))

typedef struct RouteArgs
{
    MatrixHttpHandlerArgs *matrixArgs;
    HttpServerContext *context;
    MATRIX_PATH *path;
} RouteArgs;

#define ROUTE(name) \
	extern HashMap * \
	name(RouteArgs *)

#define ROUTE_IMPL(name, argsName) \
	HashMap * \
	name(RouteArgs * argsName)

ROUTE(RouteMainPage);              /* / */
ROUTE(RouteWellKnown);             /* /.well-known */
ROUTE(RouteMatrix);                /* /_matrix */

ROUTE(RouteLogin);                 /* /_matrix/client/(r0|v3)/login */
ROUTE(RouteRegister);              /* /_matrix/client/(r0|v3)/register */
ROUTE(RouteRefresh);               /* /_matrix/client/(r0|v3)/refresh */

#undef ROUTE

#endif
