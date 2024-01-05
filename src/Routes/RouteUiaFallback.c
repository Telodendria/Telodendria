/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net>
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
#include <Cytoplasm/Json.h>
#include <Config.h>
#include <Uia.h>
#include <Cytoplasm/Str.h>

ROUTE_IMPL(RouteUiaFallback, path, argp)
{
    RouteArgs *args = argp;
    Stream *stream = HttpServerStream(args->context);
    HashMap *requestParams = HttpRequestParams(args->context);
    char *authType = ArrayGet(path, 0);
    char *sessionId;

    char *msg;

    if (!authType)
    {
        /* This should never happen */
        Log(LOG_ERR, "Programmer error in RouteUiaFallback()!");
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, NULL);
    }

    if (HttpRequestMethodGet(args->context) == HTTP_POST)
    {
        HashMap *request;
        HashMap *response;
        int uiaResult;
        Config *config;
        Array *flows;
        Array *flow;

        config = ConfigLock(args->matrixArgs->db);
        if (!config)
        {
            msg = "Internal server error: failed to lock configuration.";
            Log(LOG_ERR, "UIA fallback failed to lock configuration.");
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            return MatrixErrorCreate(M_UNKNOWN, msg);
        }

        request = JsonDecode(HttpServerStream(args->context));
        if (!request)
        {
            ConfigUnlock(config);
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_NOT_JSON, NULL);
        }

        flow = ArrayCreate();
        flows = ArrayCreate();
        ArrayAdd(flow, UiaStageBuild(authType, NULL));
        ArrayAdd(flows, flow);
        uiaResult = UiaComplete(flows, args->context,
                    args->matrixArgs->db, request, &response, config);
        UiaFlowsFree(flows);

        if (uiaResult < 0)
        {
            HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
            response = MatrixErrorCreate(M_UNKNOWN, NULL);
        }
        else if (uiaResult)
        {
            response = HashMapCreate();
        }

        JsonFree(request);
        ConfigUnlock(config);
        return response;
    }
    else if (HttpRequestMethodGet(args->context) != HTTP_GET)
    {
        msg = "Route only supports GET.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNRECOGNIZED, msg);
    }

    sessionId = HashMapGet(requestParams, "session");
    if (!sessionId)
    {
        msg = "'session' parameter is unset.";
        HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_MISSING_PARAM, msg);
    }

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);
    HtmlBegin(stream, "Authentication");

    if (StrEquals(authType, "m.login.password"))
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
        StreamPrintf(stream,
                 "function buildRequest() {"
                 "  let user = document.getElementById('user').value;"
                 "  let pass = document.getElementById('password').value;"
                 "  if (!user || !pass) {"
                 "    setFormError('Please specify a username and password.');"
                 "    return false;"
                 "  }"
                 "  return {"
                 "    auth: {"
                 "      type: '%s',"
                 "      identifier: {"
                 "        type: 'm.id.user',"
                 "        user: user"
                 "      },"
                 "      password: pass,"
                 "      session: '%s'"
                 "    }"
                 "  };"
                 "}", authType, sessionId);
        HtmlEndJs(stream);
    }
    else if (StrEquals(authType, "m.login.registration_token"))
    {
        HtmlBeginForm(stream, "auth-form");
        StreamPuts(stream,
                   "<label for=\"token\">Registration Token:</label>"
                   "<input type=\"password\" id=\"token\">"
                   "<br>"
                   "<input type=\"submit\" value=\"Authenticate\">");
        HtmlEndForm(stream);
        HtmlBeginJs(stream);
        StreamPrintf(stream,
                     "function buildRequest() {"
               "  let token = document.getElementById('token').value;"
                     "  if (!token) { "
           "    setFormError('Please specify a registration token.');"
                     "    return false;"
                     "  }"
                     "  return {"
                     "    auth: {"
                     "      type: '%s',"
                     "      session: '%s',"
                     "      token: token"
                     "    }"
                     "  };"
                     "}", authType, sessionId);
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
    StreamPrintf(stream,
                 "function processResponse(xhr) {"
                 "  let r = JSON.parse(xhr.responseText);"
                 "  console.log(r);"
                 "  if (xhr.status == 200 || r.completed.includes('%s')) {"
                 "    if (window.onAuthDone) {"
                 "      window.onAuthDone();"
                 "    } else if (window.opener && window.opener.postMessage) {"
                 "      window.opener.postMessage('authDone', '*');"
                 "    } else {"
                 "      setFormError('Client error.');"
                 "    }"
                 "  } else if (r.session != '%s') {"
                 "    setFormError('Invalid session.');"
                 "  } else {"
                 "    setFormError('Invalid credentials.');"
                 "  }"
                 "}", authType, sessionId);

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
