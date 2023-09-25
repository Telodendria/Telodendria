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

/***
 * @Nm Routes
 * @Nd Matrix API endpoint handler functions.
 * @Dd April 28 2023
 * @Xr Matrix HttpRouter
 *
 * .Nm
 * provides all of the Matrix API route functions, which for the sake
 * of brevity are not documented here---consult the official Matrix
 * specification for documentation on Matrix routes and the
 * .Xr telodendria-admin
 * page for admin API routes.
 *
 * @suppress-warnings
 */

#include <HashMap.h>
#include <Array.h>
#include <HttpServer.h>
#include <HttpRouter.h>
#include <Matrix.h>

#include <string.h>

/**
 * Every route function takes this structure, which contains the data
 * it needs to successfully handle an API request.
 */
typedef struct RouteArgs
{
    MatrixHttpHandlerArgs *matrixArgs;
    HttpServerContext *context;
} RouteArgs;

/**
 * Build an HTTP router that sets up all the route functions to be
 * executed at the correct HTTP paths.
 */
extern HttpRouter * RouterBuild(void);

#define ROUTE(name) \
	extern void * \
	name(Array *, void *)

#define ROUTE_IMPL(name, path, args) \
	void * \
	name(Array * path, void * args)

ROUTE(RouteVersions);
ROUTE(RouteWellKnown);

ROUTE(RouteCapabilities);
ROUTE(RouteLogin);
ROUTE(RouteLogout);
ROUTE(RouteRegister);
ROUTE(RouteRefresh);
ROUTE(RouteWhoami);
ROUTE(RouteChangePwd);
ROUTE(RouteDeactivate);
ROUTE(RouteTokenValid);
ROUTE(RouteUserProfile);
ROUTE(RouteRequestToken);

ROUTE(RouteUiaFallback);
ROUTE(RouteStaticDefault);
ROUTE(RouteStaticLogin);
ROUTE(RouteStaticResources);

ROUTE(RouteFilter);

ROUTE(RouteProcControl);
ROUTE(RouteConfig);
ROUTE(RoutePrivileges);

ROUTE(RouteCreateRoom);

ROUTE(RouteAliasDirectory);
ROUTE(RouteRoomAliases);

ROUTE(RouteAdminDeactivate);

#undef ROUTE

#endif
