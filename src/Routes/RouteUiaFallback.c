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
#include <Html.h>

ROUTE_IMPL(RouteUiaFallback, path, argp)
{
    RouteArgs *args = argp;
    Stream *stream = HttpServerStream(args->context);
    HashMap *requestParams = HttpRequestParams(args->context);
    char *authType = ArrayGet(path, 0);
    char *sessionId;

    if (!authType)
    {
        /* This should never happen */
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN);
    }

    sessionId = HashMapGet(requestParams, "session");
    if (!sessionId)
    {
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_MISSING_PARAM);
    }

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);
    HtmlBegin(stream, "Authentication");

    if (strcmp(authType, "m.login.password") == 0)
    {
        HtmlBeginForm(stream, "auth-form");
        StreamPuts(stream,
                   "<label for=\"user\">Username:</label>"
                   "<input type=\"text\" id=\"user\">"
                   "<label for=\"password\">Password:</label>"
                   "<input type=\"password\" id=\"password\">"
                   "<br>"
                   "<input type=\"submit\" value=\"Authenticate\">");
        HtmlEndForm(stream);
        HtmlBeginJs(stream);
        /* TODO */
        StreamPuts(stream,
                   "function buildRequest() {"
                   "  setFormError('Not implemented yet.');"
                   "  return false;"
                   "}");
        HtmlEndJs(stream);
    }
    else if (strcmp(authType, "m.login.registration_token") == 0)
    {
        HtmlBeginForm(stream, "auth-form");
        StreamPuts(stream,
                   "<label for=\"token\">Registration Token:</label>"
                   "<input type=\"text\" id=\"token\">"
                   "<br>"
                   "<input type=\"submit\" value=\"Authenticate\">");
        HtmlEndForm(stream);
        HtmlBeginJs(stream);
        /* TODO */
        StreamPuts(stream,
                   "function buildRequest() {"
                   "  setFormError('Not implemented yet.');"
                   "  return false;"
                   "}");
        HtmlEndJs(stream);
    }
    /*
     * TODO: implement m.login.recaptcha, m.login.sso,
     * m.login.email.identity, m.login.msisdn here
     */
    else
    {
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        StreamPrintf(stream,
               "<p>Unknown auth type: <code>%s</code></p>", authType);
        goto finish;
    }

    HtmlBeginJs(stream);
    StreamPuts(stream,
               "function processResponse(xhr) {"
               "  if (xhr.status == 200) {"
               "    if (window.onAuthDone) {"
               "      window.onAuthDone();"
        "    } else if (window.opener && window.opener.postMessage) {"
               "      window.opener.postMessage('authDone', '*');"
               "    } else {"
               "      setFormError('Client error.');"
               "    }"
               "  } else {"
               "    let r = JSON.parse(xhr.responseText);"
               "    setFormError(`${r.errcode}: ${r.error}`);"
               "  }"
               "}");

    StreamPuts(stream,
               "onFormSubmit('auth-form', (frm) => {"
               "  let request = buildRequest();"
               "  if (request) {"
               "    jsonRequest('POST', window.location.pathname, request, processResponse);"
               "  }"
               "});");
    HtmlEndJs(stream);

finish:
    HtmlEnd(stream);
    return NULL;
}
