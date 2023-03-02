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
#include <Html.h>

#include <stdio.h>

#include <Telodendria.h>

void
HtmlBegin(FILE * stream, char *title)
{
    size_t i;

    if (!stream)
    {
        return;
    }

    fprintf(stream,
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>%s | Telodendria</title>"
            ,title
            );
    HtmlBeginStyle(stream);
    fprintf(stream,
            ":root {"
            "  color-scheme: dark;"
            "  --accent: #7b8333;"
            "}"
            "body {"
            "  margin: auto;"
            "  width: 100%%;"
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
            "}"
            );

    fprintf(stream,
            ".form {"
            "  margin: auto;"
            "  width: 100%%;"
            "  max-width: 400px;"
            "  border-radius: 10px;"
            "  border: 1px var(--accent) solid;"
            "  padding: 10px;"
            "}"
            "form {"
            "  display: block;"
            "}"
            "form > input, label {"
            "  width: 95%%;"
            "  height: 25px;"
            "  display: block;"
            "  margin-bottom: 5px;"
            "  margin-left: auto;"
            "  margin-right: auto;"
            "}"
            );
    HtmlEndStyle(stream);

    fprintf(stream,
            "</head>"
            "<body>"
            "<pre class=\"logo\">"
            );

    for (i = 0; i < TELODENDRIA_LOGO_HEIGHT; i++)
    {
        fprintf(stream, "%s\n", TelodendriaLogo[i]);
    }

    fprintf(stream,
            "</pre>"
            "<h1>%s</h1>"
            ,title);
}

void
HtmlEnd(FILE * stream)
{
    fprintf(stream,
            "</body>"
            "</html>");
}
