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
#ifndef TELODENDRIA_HTML_H
#define TELODENDRIA_HTML_H

/***
 * @Nm Html
 * @Nd Utility functions for generating static HTML pages.
 * @Dd April 27 2023
 * 
 * .Nm
 * provides some simple macros and functions for generating HTML
 * pages. These are very specific to Telodendria, as they automatically
 * apply the color scheme and make assumptions about the stylesheets
 * and scripts included.
 * .Pp
 * The following macros are available:
 * .Bl -tag -width Ds
 * .It HtmlBeginJs(stream)
 * Begin JavaScript output. This sets up the opening script tags, and
 * licenses the following JavaScript under the MIT license.
 * .It HtmlEndJs(stream)
 * End JavaScript output.
 * .It HtmlBeginStyle(stream)
 * Begin CSS output. This sets up the opening syle tags.
 * .It HtmlEndStyle(stream)
 * End CSS output.
 * .It HtmlBeginForm(stream, id)
 * Begin a new form with the specified ID. This sets up the opening
 * form tags, which includes placing the form in a div with class
 * 'form'.
 * .It HtmlEndForm(stream)
 * End HTML form output.
 * .El
 */

#include <Cytoplasm/Stream.h>

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

/**
 * Initialize an HTML page by writing the head and the beginning of the
 * body. After this function is called, the page's main HTML can be
 * written. This function takes the name of the page it is beginning.
 * The name is placed in the title tags, and is used as the page
 * header.
 */
extern void HtmlBegin(Stream *, char *);

/**
 * Finish an HTML page by writing any necessary closing tags.
 */
extern void HtmlEnd(Stream *);

#endif                             /* TELODENDRIA_HTML_H */
