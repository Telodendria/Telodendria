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
#include <HttpRouter.h>
#include <Matrix.h>

#define MATRIX_PATH_EQUALS(pathPart, str) \
	((pathPart != NULL) && (strcmp(pathPart, str) == 0))

typedef struct RouteArgs
{
    MatrixHttpHandlerArgs *matrixArgs;
    HttpServerContext *context;
} RouteArgs;

HttpRouter *
 RouterBuild(void);

#define ROUTE(name) \
	extern void * \
	name(Array *, void *)

#define ROUTE_IMPL(name, matchesName, argsName) \
	void * \
	name(Array * matchesName, void * argsName)

ROUTE(RouteVersions);
ROUTE(RouteWellKnown);

ROUTE(RouteLogin);
ROUTE(RouteLogout);
ROUTE(RouteRegister);
ROUTE(RouteRefresh);
ROUTE(RouteWhoami);
ROUTE(RouteChangePwd);
ROUTE(RouteTokenValid);
ROUTE(RouteUserProfile);
ROUTE(RouteRequestToken);

ROUTE(RouteUiaFallback);
ROUTE(RouteStaticDefault);
ROUTE(RouteStaticLogin);

ROUTE(RouteProcControl);
ROUTE(RouteConfig);

#undef ROUTE

#endif
