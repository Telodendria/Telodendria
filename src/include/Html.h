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
#ifndef TELODENDRIA_HTML_H
#define TELODENDRIA_HTML_H

#include <Stream.h>

#define HtmlBeginJs(stream) StreamPuts(stream, \
        "<script>" \
        "\n// @license magnet:?xt=" \
        "urn:btih:d3d9a9a6595521f9666a5e94cc830dab83b65699&dn=expat.txt " \
        "Expat\n")

#define HtmlEndJs(stream) StreamPuts(stream, "\n// @license-end\n</script>")

#define HtmlBeginStyle(stream) StreamPuts(stream, "<style>")
#define HtmlEndStyle(stream) StreamPuts(stream, "</style>")

#define HtmlBeginForm(stream, id) StreamPrintf(stream, \
        "<div class=\"form\">" \
        "<form id=\"%s\">", id);

#define HtmlEndForm(stream) StreamPuts(stream, \
        "</form>" \
        "<p id=\"error-msg\"></p>" \
        "</div>");

extern void
 HtmlBegin(Stream *, char *);

extern void
 HtmlEnd(Stream *);

#endif                             /* TELODENDRIA_HTML_H */
