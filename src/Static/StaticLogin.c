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
#include <Static.h>
#include <Html.h>

void
StaticLogin(FILE * stream)
{
    HtmlBegin(stream, "Log In");

    fprintf(stream,
            "<div class=\"form\">"
            "<form id=\"login-form\">"
            "<label for=\"user\">Username:</label>"
            "<input type=\"text\" id=\"user\">"
            "<label for=\"password\">Password:</label>"
            "<input type=\"password\" id=\"password\">"
            "<br>"
            "<input type=\"submit\" value=\"Log In\">"
            "</form>"
            "<style>"
            "#error-msg {"
            "  display: none;"
            "  color: red;"
            "  text-align: center;"
            "  font-weight: bold;"
            "  font-size: larger;"
            "}"
            "</style>"
            "<p id=\"error-msg\"></p>"
            "</div>"
            );

    fprintf(stream,
            "<script>"
            "function findGetParameter(parameterName) {"
            "  var result = null;"
            "  var tmp = [];"
            "  var items = location.search.substr(1).split(\"&\");"
            "  for (var index = 0; index < items.length; index++) {"
            "    tmp = items[index].split(\"=\");"
            "    if (tmp[0] === parameterName) result = decodeURIComponent(tmp[1]);"
            "  }"
            "  return result;"
            "}"
            "function setError(msg) {"
            "  var err = document.getElementById('error-msg');"
            "  if (msg) {"
            "    err.style.display = 'block';"
            "    err.innerHTML = msg;"
            "  } else {"
            "    err.style.display = 'none';"
            "  }"
            "}"
            );

    fprintf(stream,
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

    fprintf(stream,
            "function processResponse(xhr) {"
            "  if (xhr.readyState == 4) {"
            "    var r = JSON.parse(xhr.responseText);"
            "    if (!r.error) {"
            "      if (window.onLogin) {"
            "        window.onLogin(r);"
            "      } else {"
            "        setError('Client malfunction. Your client should have defined window.onLogin()');"
            "      }"
            "    } else {"
            "      setError(r.errcode + ': ' + r.error);"
            "    }"
            "  }"
            "}"
            );

    fprintf(stream,
            "function sendRequest(request) {"
            "  var xhr = new XMLHttpRequest();"
            "  xhr.open('POST', '/_matrix/client/v3/login');"
         "  xhr.setRequestHeader('Content-Type', 'application/json');"
            "  xhr.onreadystatechange = () => processResponse(xhr);"
            "  xhr.send(JSON.stringify(request));"
            "}"
            );

    fprintf(stream,
            "window.addEventListener('load', () => {"
            "  document.getElementById('login-form').addEventListener('submit', (e) => {"
            "    e.preventDefault();"
            "    var user = document.getElementById('user').value;"
            "    var pass = document.getElementById('password').value;"
            "    if (!user || !pass) {"
          "      setError('Please provide a username and password.');"
            "      return;"
            "    }"
            "    setError(null);"
            "    var request = buildRequest(user, pass);"
            "    sendRequest(request);"
            "  });"
            "});"
            "</script>"
            );

    HtmlEnd(stream);
}
