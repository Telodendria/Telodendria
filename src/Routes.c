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

HttpRouter *
RouterBuild(void)
{
    HttpRouter *router = HttpRouterCreate();

    if (!router)
    {
        return NULL;
    }

#define R(path, func) \
    if (!HttpRouterAdd(router, path, func)) \
    { \
        Log(LOG_ERR, "Unable to add route: %s", path); \
        HttpRouterFree(router); \
        return NULL; \
    }

    /* Matrix Specifification Routes */

    R("/.well-known/matrix/(client|server)", RouteWellKnown);

    R("/_matrix/client/versions", RouteVersions);

    R("/_matrix/static", RouteStaticDefault);
    R("/_matrix/static/telodendria\\.(js|css)", RouteStaticResources);
    R("/_matrix/static/client/login", RouteStaticLogin);
    R("/_matrix/client/v3/auth/(.*)/fallback/web", RouteUiaFallback);

    R("/_matrix/client/v3/capabilities", RouteCapabilities);
    R("/_matrix/client/v3/login", RouteLogin);
    R("/_matrix/client/v3/logout", RouteLogout);
    R("/_matrix/client/v3/logout/(all)", RouteLogout);
    R("/_matrix/client/v3/register", RouteRegister);
    R("/_matrix/client/v3/register/(available)", RouteRegister);
    R("/_matrix/client/v3/refresh", RouteRefresh);

    R("/_matrix/client/v3/account/whoami", RouteWhoami);
    R("/_matrix/client/v3/account/password", RouteChangePwd);
    R("/_matrix/client/v3/account/deactivate", RouteDeactivate);

    R("/_matrix/client/v1/register/m.login.registration_token/validity", RouteTokenValid);

    R("/_matrix/client/v3/account/password/(email|msisdn)/requestToken", RouteRequestToken);
    R("/_matrix/client/v3/register/(email|msisdn)/requestToken", RouteRequestToken);

    R("/_matrix/client/v3/profile/(.*)", RouteUserProfile);
    R("/_matrix/client/v3/profile/(.*)/(avatar_url|displayname)", RouteUserProfile);

    R("/_matrix/client/v3/user/(.*)/filter", RouteFilter);
    R("/_matrix/client/v3/user/(.*)/filter/(.*)", RouteFilter);

    /* Telodendria Admin API Routes */

    R("/_telodendria/admin/(restart|shutdown|stats)", RouteProcControl);
    R("/_telodendria/admin/config", RouteConfig);
    R("/_telodendria/admin/privileges", RoutePrivileges);
    R("/_telodendria/admin/privileges/(.*)", RoutePrivileges);

#undef R

    return router;
}
