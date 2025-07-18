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
#include <Routes.h>
#include <Html.h>

ROUTE_IMPL(RouteStaticDefault, path, argp)
{
    RouteArgs *args = argp;
    Stream *stream = HttpServerStream(args->context);

    (void) path;

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);
    HtmlBegin(stream, "It works! Telodendria is running.");

    StreamPuts(stream,
               "<style>"
               "p {"
               "  text-align: center;"
               "}"
               "</style>"
            );

    StreamPuts(stream,
               "<p>"
     "Your Telodendria server is listening on this port and is ready "
               "for messages."
               "</p>"
               "<p>"
               "To use this server, you'll need <a href=\"https://matrix.org/clients\">"
               "a Matrix client</a>."
               "</p>"
               "<p>"
               "Welcome to the Matrix universe :)"
               "</p>"
            );

    HtmlEnd(stream);

    return NULL;
}
