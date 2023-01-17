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

#include <Telodendria.h>

ROUTE_IMPL(RouteMainPage, args)
{
    FILE *stream = HttpStream(args->context);
    size_t i;

    HttpResponseHeader(args->context, "Content-Type", "text/html");
    HttpSendHeaders(args->context);

    fprintf(stream,
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>It Works! | Telodendria</title>"
            "<style>"
            "body {"
            "  margin: auto;"
            "  max-width: 6in;"
            "  padding: 0.25in;"
            "  background-color: #0d1117;"
            "  color: white;"
            "}"
            "h1, p, pre {"
            "  text-align: center;"
            "}"
            "a {"
            "  color: #7b8333;"
            "  text-decoration: none;"
            "}"
            "</style>"
            "</head>"
            "<body>"
            "<pre style=\"color: #7b8333; font-weight: bold;\">"
            );

    for (i = 0; i < TELODENDRIA_LOGO_HEIGHT; i++)
    {
        fprintf(stream, "%s\n", TelodendriaLogo[i]);
    }

    fprintf(stream,
            "</pre>"
            "<h1>It works! Telodendria is running</h1>"
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

    fprintf(stream,
            "</body>"
            "</html>"
            );

    return NULL;
}
