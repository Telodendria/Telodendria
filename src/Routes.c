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

    R("/.well-known/matrix/(client|server)", RouteWellKnown);

    R("/_matrix/client/versions", RouteVersions);

    R("/_matrix/static", RouteStaticDefault);
    R("/_matrix/static/client/login", RouteStaticLogin);

    R("/_matrix/client/v3/login", RouteLogin);
    R("/_matrix/client/v3/logout", RouteLogout);
    R("/_matrix/client/v3/logout/(all)", RouteLogout);
    R("/_matrix/client/v3/register", RouteRegister);
    R("/_matrix/client/v3/register/(available)", RouteRegister);
    R("/_matrix/client/v3/refresh", RouteRefresh);

    R("/_matrix/client/v3/account/whoami", RouteWhoami);
    R("/_matrix/client/v3/account/password", RouteChangePwd);

    R("/_matrix/client/v1/register/m.login.registration_token/validity", RouteTokenValid);

#if 0
    R("/_matrix/client/v3/account/password/(email|msisdn)/requestToken", RouteRequestToken);
    R("/_matrix/client/v3/register/(email|msisdn)/requestToken", RouteRequestToken);
#endif

    R("/_matrix/client/v3/profile/(.*)", RouteUserProfile);
    R("/_matrix/client/v3/profile/(.*)/(avatar_url|displayname)", RouteUserProfile);

#undef R

    return router;
}
