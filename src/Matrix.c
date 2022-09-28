/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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
#include <stdlib.h>

#include <HttpServer.h>
#include <Json.h>
#include <Util.h>

void
MatrixHttpHandler(HttpServerContext * context, void *argp)
{
    MatrixHttpHandlerArgs *args = (MatrixHttpHandlerArgs *) argp;

    LogConfig *lc = args->lc;

    HashMap *requestHeaders = HttpRequestHeaders(context);
    FILE *stream;

    char *key;
    char *val;
    size_t i;

    HashMap *response;

    char *requestPath;
    Array *pathParts;
    char *pathPart;

    requestPath = HttpRequestPath(context);

    Log(lc, LOG_MESSAGE, "%s %s",
        HttpRequestMethodToString(HttpRequestMethodGet(context)),
        requestPath);

    LogConfigIndent(lc);
    Log(lc, LOG_DEBUG, "Request headers:");

    LogConfigIndent(lc);
    while (HashMapIterate(requestHeaders, &key, (void **) &val))
    {
        Log(lc, LOG_DEBUG, "%s: %s", key, val);
    }
    LogConfigUnindent(lc);

    HttpResponseStatus(context, HTTP_OK);
    HttpResponseHeader(context, "Server", "Telodendria v" TELODENDRIA_VERSION);
    HttpResponseHeader(context, "Content-Type", "application/json");

    /* CORS */
    HttpResponseHeader(context, "Access-Control-Allow-Origin", "*");
    HttpResponseHeader(context, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    HttpResponseHeader(context, "Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Authorization");

    /*
     * Web Browser Clients: Servers MUST expect that clients will approach them
     * with OPTIONS requests... the server MUST NOT perform any logic defined
     * for the endpoints when approached with an OPTIONS request.
     */
    if (HttpRequestMethodGet(context) == HTTP_OPTIONS)
    {
        HttpResponseStatus(context, HTTP_NO_CONTENT);
        HttpSendHeaders(context);

        goto finish;
    }

    pathParts = ArrayCreate();
    key = requestPath;

    while ((pathPart = strtok_r(key, "/", &key)))
    {
        char *decoded = HttpUrlDecode(pathPart);

        ArrayAdd(pathParts, decoded);
    }

    /* TODO: Route requests here */

    HttpSendHeaders(context);
    stream = HttpStream(context);

    response = MatrixErrorCreate(M_UNKNOWN);
    JsonEncode(response, stream);
    fprintf(stream, "\n");

    for (i = 0; i < ArraySize(pathParts); i++)
    {
        free(ArrayGet(pathParts, i));
    }

    ArrayFree(pathParts);

    JsonFree(response);

finish:
    stream = HttpStream(context);
    fclose(stream);

    LogConfigUnindent(lc);
}

HashMap *
MatrixErrorCreate(MatrixError errorArg)
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

    errorObj = HashMapCreate();
    if (!errorObj)
    {
        return NULL;
    }

    HashMapSet(errorObj, "errcode", JsonValueString(UtilStringDuplicate(errcode)));
    HashMapSet(errorObj, "error", JsonValueString(UtilStringDuplicate(error)));

    return errorObj;
}
