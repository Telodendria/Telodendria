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
#include <Html.h>

#include <stdio.h>

#include <Telodendria.h>

void
HtmlBegin(Stream * stream, char *title)
{
    size_t i;

    if (!stream)
    {
        return;
    }

    StreamPrintf(stream,
                 "<!DOCTYPE html>"
                 "<html>"
                 "<head>"
                 "<meta charset=\"utf-8\">"
                 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                 "<title>%s | Telodendria</title>"
                 "<link rel=\"stylesheet\" href=\"/_matrix/static/telodendria.css\">"
            "<script src=\"/_matrix/static/telodendria.js\"></script>"
                 "</head>"
                 "<body>"
                 "<pre class=\"logo\">"
                 ,title
            );

    for (i = 0; i < TELODENDRIA_LOGO_HEIGHT; i++)
    {
        StreamPrintf(stream, "%s\n", TelodendriaLogo[i]);
    }

    StreamPrintf(stream,
                 "</pre>"
                 "<h1>%s</h1>"
                 ,title);
}

void
HtmlEnd(Stream * stream)
{
    StreamPuts(stream,
               "</body>"
               "</html>");
}
