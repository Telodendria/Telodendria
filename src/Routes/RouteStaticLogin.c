/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

ROUTE_IMPL(RouteStaticLogin, path, argp)
{
    RouteArgs *args = argp;
    Stream *stream = HttpServerStream(args->context);

    (void) path;

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);

    HtmlBegin(stream, "Log In");

    HtmlBeginForm(stream, "login-form");
    StreamPuts(stream,
               "<label for=\"user\">Username:</label>"
               "<input type=\"text\" id=\"user\">"
               "<label for=\"password\">Password:</label>"
               "<input type=\"password\" id=\"password\">"
               "<br>"
               "<input type=\"submit\" value=\"Log In\">"
            );
    HtmlEndForm(stream);

    HtmlBeginJs(stream);
    StreamPuts(stream,
               "function buildRequest(user, pass) {"
               "  var d = findGetParameter('device_id');"
          "  var i = findGetParameter('initial_device_display_name');"
             "  var r = findGetParameter('refresh_token') === 'true';"
               "  var request = {};"
               "  request['type'] = 'm.login.password';"
               "  request['identifier'] = {"
               "    type: 'm.id.user',"
               "    user: user"
               "  };"
               "  request['password'] = pass;"
               "  if (d) request['device_id'] = d;"
               "  if (i) request['initial_device_display_name'] = i;"
               "  if (r) request['refresh_token'] = r;"
               "  return request;"
               "}"
            );

    StreamPuts(stream,
               "function processResponse(xhr) {"
               "  if (xhr.readyState == 4) {"
               "    var r = JSON.parse(xhr.responseText);"
               "    if (!r.error) {"
               "      if (window.onLogin) {"
               "        window.onLogin(r);"
               "      } else {"
               "        setFormError('Client error.');"
               "      }"
               "    } else {"
               "      setFormError(r.errcode + ': ' + r.error);"
               "    }"
               "  }"
               "}"
            );

    StreamPuts(stream,
               "onFormSubmit('login-form', (frm) => {"
               "  var user = document.getElementById('user').value;"
             "  var pass = document.getElementById('password').value;"
               "  if (!user || !pass) {"
        "    setFormError('Please provide a username and password.');"
               "    return;"
               "  }"
               "  setFormError(null);"
               "  var request = buildRequest(user, pass);"
               "  jsonRequest('POST', '/_matrix/client/v3/login', request, processResponse);"
               "});"
            );
    HtmlEndJs(stream);

    HtmlEnd(stream);

    return NULL;
}
