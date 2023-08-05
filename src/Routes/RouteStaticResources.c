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

#include <Str.h>

ROUTE_IMPL(RouteStaticResources, path, argp)
{
    RouteArgs *args = argp;
    Stream *stream = HttpServerStream(args->context);
    char *res = ArrayGet(path, 0);

    if (!res)
    {
        /* Should be impossible */
        HttpResponseStatus(args->context, HTTP_INTERNAL_SERVER_ERROR);
        return MatrixErrorCreate(M_UNKNOWN, NULL);
    }

    if (StrEquals(res, "js"))
    {
        HttpResponseHeader(args->context, "Content-Type", "text/javascript");
        HttpSendHeaders(args->context);

        StreamPuts(stream,
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
                   "function setFormError(msg) {"
                   "  var err = document.getElementById('error-msg');"
                   "  if (msg) {"
                   "    err.style.display = 'block';"
                   "    err.innerHTML = msg;"
                   "  } else {"
                   "    err.style.display = 'none';"
                   "  }"
                   "}"
                );
        StreamPuts(stream,
                   "function jsonRequest(meth, url, json, cb) {"
                   "  var xhr = new XMLHttpRequest();"
                   "  xhr.open(meth, url);"
         "  xhr.setRequestHeader('Content-Type', 'application/json');"
                   "  xhr.onreadystatechange = () => {"
                   "    if (xhr.readyState == 4) {"
                   "      cb(xhr);"
                   "    }"
                   "  };"
                   "  xhr.send(JSON.stringify(json));"
                   "}"
                   "function onFormSubmit(frm, cb) {"
                   "  window.addEventListener('load', () => {"
                   "    frm = document.getElementById(frm);"
                   "    frm.addEventListener('submit', (e) => {"
                   "      e.preventDefault();"
                   "      cb(frm);"
                   "    });"
                   "  });"
                   "}"
                );

    }
    else if (StrEquals(res, "css"))
    {
        HttpResponseHeader(args->context, "Content-Type", "text/css");
        HttpSendHeaders(args->context);
        StreamPuts(stream,
                   ":root {"
                   "  color-scheme: dark;"
                   "  --accent: #7b8333;"
                   "}"
                   "body {"
                   "  margin: auto;"
                   "  width: 100%;"
                   "  max-width: 8.5in;"
                   "  padding: 0.25in;"
                   "  background-color: #0d1117;"
                   "  color: white;"
                   "}"
                   "a {"
                   "  color: var(--accent);"
                   "  text-decoration: none;"
                   "}"
                   "h1 {"
                   "  text-align: center;"
                   "}"
                   ".logo {"
                   "  color: var(--accent);"
                   "  text-align: center;"
                   "  font-weight: bold;"
                   "}");
        StreamPuts(stream,
                   ".form {"
                   "  margin: auto;"
                   "  width: 100%;"
                   "  max-width: 400px;"
                   "  border-radius: 10px;"
                   "  border: 1px var(--accent) solid;"
                   "  padding: 10px;"
                   "}"
                   "form {"
                   "  display: block;"
                   "}"
                   "form > input, label {"
                   "  width: 95%;"
                   "  height: 25px;"
                   "  display: block;"
                   "  margin-bottom: 5px;"
                   "  margin-left: auto;"
                   "  margin-right: auto;"
                   "}"
                   ".form > #error-msg {"
                   "  display: none;"
                   "  color: red;"
                   "  text-align: center;"
                   "  font-weight: bold;"
                   "  font-size: larger;"
                   "}");
    }
    else
    {
        HttpResponseStatus(args->context, HTTP_NOT_FOUND);
        return MatrixErrorCreate(M_NOT_FOUND, NULL);
    }


    return NULL;
}
