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

#include <Matrix.h>

#include <string.h>
#include <ctype.h>

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/HttpServer.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Str.h>

#include <Cytoplasm/HttpRouter.h>
#include <Routes.h>

void
MatrixHttpHandler(HttpServerContext * context, void *argp)
{
    MatrixHttpHandlerArgs *args = (MatrixHttpHandlerArgs *) argp;
    Stream *stream;
    HashMap *response = NULL;

    char *requestPath;
    RouteArgs routeArgs;

    requestPath = HttpRequestPath(context);
    stream = HttpServerStream(context);

    Log(LOG_DEBUG, "%s %s",
        HttpRequestMethodToString(HttpRequestMethodGet(context)),
        requestPath);

    HttpResponseStatus(context, HTTP_OK);
    HttpResponseHeader(context, "Server", "Telodendria/" TELODENDRIA_VERSION);

    /* CORS */
    HttpResponseHeader(context, "Access-Control-Allow-Origin", "*");
    HttpResponseHeader(context, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    HttpResponseHeader(context, "Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Authorization");

    HttpResponseHeader(context, "Connection", "close");

    /*
     * Web Browser Clients: Servers MUST expect that clients will approach them
     * with OPTIONS requests... the server MUST NOT perform any logic defined
     * for the endpoints when approached with an OPTIONS request.
     */
    if (HttpRequestMethodGet(context) == HTTP_OPTIONS)
    {
        HttpResponseStatus(context, HTTP_NO_CONTENT);
        HttpSendHeaders(context);

        return;
    }

    routeArgs.matrixArgs = args;
    routeArgs.context = context;

    if (!HttpRouterRoute(args->router, requestPath, &routeArgs, (void **) &response))
    {
        HttpResponseHeader(context, "Content-Type", "application/json");
        HttpResponseStatus(context, HTTP_NOT_FOUND);
        response = MatrixErrorCreate(M_NOT_FOUND, NULL);
    }

    /*
     * If the route handler returned a JSON object, take care
     * of sending it here.
     *
     * Otherwise, if the route handler returned NULL, assume
     * that it sent its own headers and and body.
     */
    if (response)
    {
        char *contentLen = StrInt(JsonEncode(response, NULL, JSON_DEFAULT));

        HttpResponseHeader(context, "Content-Type", "application/json");
        HttpResponseHeader(context, "Content-Length", contentLen);
        HttpSendHeaders(context);

        Free(contentLen);

        JsonEncode(response, stream, JSON_DEFAULT);
        JsonFree(response);
        StreamPrintf(stream, "\n");
    }

    Log(LOG_INFO, "%s %s (%d %s)",
        HttpRequestMethodToString(HttpRequestMethodGet(context)),
        requestPath,
        HttpResponseStatusGet(context),
        HttpStatusToString(HttpResponseStatusGet(context)));
}

HashMap *
MatrixErrorCreate(MatrixError errorArg, char *msg)
{
    HashMap *errorObj;
    char *errcode;
    char *error;

    switch (errorArg)
    {
        case M_FORBIDDEN:
            errcode = "M_FORBIDDEN";
            error = "Forbidden access. Bad permissions or not authenticated.";
            break;
        case M_UNKNOWN_TOKEN:
            errcode = "M_UNKNOWN_TOKEN";
            error = "The access or refresh token specified was not recognized.";
            break;
        case M_MISSING_TOKEN:
            errcode = "M_MISSING_TOKEN";
            error = "No access token was specified for the request.";
            break;
        case M_BAD_JSON:
            errcode = "M_BAD_JSON";
            error = "Request contained valid JSON, but it was malformed in some way.";
            break;
        case M_NOT_JSON:
            errcode = "M_NOT_JSON";
            error = "Request did not contain valid JSON.";
            break;
        case M_NOT_FOUND:
            errcode = "M_NOT_FOUND";
            error = "No resource was found for this request.";
            break;
        case M_LIMIT_EXCEEDED:
            errcode = "M_LIMIT_EXCEEDED";
            error = "Too many requests have been sent in a short period of time. "
                    "Wait a while then try again.";
            break;
        case M_UNKNOWN:
            errcode = "M_UNKNOWN";
            error = "An unknown error has occurred.";
            break;

        case M_UNRECOGNIZED:
            errcode = "M_UNRECOGNIZED";
            error = "The server did not understand the request.";
            break;
        case M_UNAUTHORIZED:
            errcode = "M_UNAUTHORIZED";
            error = "The request was not correctly authorized.";
            break;
        case M_USER_DEACTIVATED:
            errcode = "M_USER_DEACTIVATED";
            error = "The user ID assocated with the request has been deactivated.";
            break;
        case M_USER_IN_USE:
            errcode = "M_USER_IN_USE";
            error = "The user ID specified has already been taken.";
            break;
        case M_INVALID_USERNAME:
            errcode = "M_INVALID_USERNAME";
            error = "The user ID specified is not valid.";
            break;
        case M_ROOM_IN_USE:
            errcode = "M_ROOM_IN_USE";
            error = "The room alias given is already in use.";
            break;
        case M_INVALID_ROOM_STATE:
            errcode = "M_INVALID_ROOM_STATE";
            error = "The initial room state is invalid.";
            break;
        case M_THREEPID_IN_USE:
            errcode = "M_THREEPID_IN_USE";
            error = "The given threepid cannot be used because the same threepid is already in use.";
            break;
        case M_THREEPID_NOT_FOUND:
            errcode = "M_THREEPID_NOT_FOUND";
            error = "The given threepid cannot be used because no record matching the threepid "
                    "was found.";
            break;
        case M_THREEPID_AUTH_FAILED:
            errcode = "M_THREEPID_AUTH_FAILED";
            error = "Authentication could not be performed on the third party identifier.";
            break;
        case M_THREEPID_DENIED:
            errcode = "M_THREEPID_DENIED";
            error = "The server does not permit this third party identifier.";
            break;
        case M_SERVER_NOT_TRUSTED:
            errcode = "M_SERVER_NOT_TRUSTED";
            error = "The request used a third party server that this server does not trust.";
            break;
        case M_UNSUPPORTED_ROOM_VERSION:
            errcode = "M_UNSUPPORTED_ROOM_VERSION";
            error = "The request to create a room used a room version that the server "
                    "does not support.";
            break;
        case M_INCOMPATIBLE_ROOM_VERSION:
            errcode = "M_INCOMPATIBLE_ROOM_VERSION";
            error = "Attempted to join a room that has a version the server does not support.";
            break;
        case M_BAD_STATE:
            errcode = "M_BAD_STATE";
            error = "The state change requested cannot be performed.";
            break;
        case M_GUEST_ACCESS_FORBIDDEN:
            errcode = "M_GUEST_ACCESS_FORBIDDEN";
            error = "The room or resource does not permit guests to access it.";
            break;
        case M_CAPTCHA_NEEDED:
            errcode = "M_CAPTCHA_NEEDED";
            error = "A Captcha is required to complete the request.";
            break;
        case M_CAPTCHA_INVALID:
            errcode = "M_CAPTCHA_INVALID";
            error = "The Captcha provided did not match what was expected.";
            break;
        case M_MISSING_PARAM:
            errcode = "M_MISSING_PARAM";
            error = "A required parameter was missing from the request.";
            break;
        case M_INVALID_PARAM:
            errcode = "M_INVALID_PARAM";
            error = "A required parameter was invalid in some way.";
            break;
        case M_TOO_LARGE:
            errcode = "M_TOO_LARGE";
            error = "The request or entity was too large.";
            break;
        case M_EXCLUSIVE:
            errcode = "M_EXCLUSIVE";
            error = "The resource being requested is reserved by an application service, "
                    "or the application service making the request has not created the resource.";
            break;
        case M_RESOURCE_LIMIT_EXCEEDED:
            errcode = "M_RESOURCE_LIMIT_EXCEEDED";
            error = "The request cannot be completed because the homeserver has reached "
                    "a resource limit imposed on it.";
            break;
        case M_CANNOT_LEAVE_SERVER_NOTICE_ROOM:
            errcode = "M_CANNOT_LEAVE_SERVER_NOTICE_ROOM";
            error = "The user is unable to reject an invite to join the server notices room.";
            break;
        default:
            return NULL;
    }

    if (msg)
    {
        error = msg;
    }

    errorObj = HashMapCreate();
    if (!errorObj)
    {
        return NULL;
    }

    HashMapSet(errorObj, "errcode", JsonValueString(errcode));
    HashMapSet(errorObj, "error", JsonValueString(error));

    return errorObj;
}

HashMap *
MatrixGetAccessToken(HttpServerContext * context, char **accessToken)
{
    HashMap *params;
    char *token;

    params = HttpRequestHeaders(context);
    token = HashMapGet(params, "authorization");

    if (token)
    {
        /* If the header was provided but it's not given correctly,
         * that's an error */
        if (strncmp(token, "Bearer ", 7) != 0)
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            return MatrixErrorCreate(M_MISSING_TOKEN, NULL);
        }

        /* Seek past "Bearer" */
        token += 7;

        /* Seek past any spaces between "Bearer" and the token */
        while (*token && isspace((unsigned char) *token))
        {
            token++;
        }
    }
    else
    {
        /* Header was not provided, we must check for ?access_token */
        params = HttpRequestParams(context);
        token = HashMapGet(params, "access_token");

        if (!token)
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            return MatrixErrorCreate(M_MISSING_TOKEN, NULL);
        }
    }

    *accessToken = token;
    return NULL;
}

HashMap *
MatrixRateLimit(HttpServerContext * context, Db * db)
{
    /* TODO: Implement rate limiting */
    (void) context;
    (void) db;
    return NULL;
}

HashMap *
MatrixClientWellKnown(char *base, char *identity)
{
    HashMap *response;
    HashMap *homeserver;

    if (!base)
    {
        return NULL;
    }

    response = HashMapCreate();
    homeserver = HashMapCreate();

    HashMapSet(homeserver, "base_url", JsonValueString(base));
    HashMapSet(response, "m.homeserver", JsonValueObject(homeserver));

    if (identity)
    {
        HashMap *identityServer = HashMapCreate();

        HashMapSet(identityServer, "base_url", JsonValueString(identity));
        HashMapSet(response, "m.identity_server", JsonValueObject(identityServer));
    }

    return response;
}
