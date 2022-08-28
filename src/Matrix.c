/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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

#include <Matrix.h>

#include <HttpServer.h>

void
MatrixHttpHandler(HttpServerContext * context, void *argp)
{
    MatrixHttpHandlerArgs *args = (MatrixHttpHandlerArgs *) argp;

    LogConfig *lc = args->lc;

    HashMap *requestHeaders = HttpRequestHeaders(context);
    FILE *stream;

    char *key;
    char *val;

    Log(lc, LOG_MESSAGE, "%s %s",
        HttpRequestMethodToString(HttpRequestMethodGet(context)),
        HttpRequestPath(context));


    LogConfigIndent(lc);
    Log(lc, LOG_DEBUG, "Request headers:");

    LogConfigIndent(lc);
    while (HashMapIterate(requestHeaders, &key, (void **) &val))
    {
        Log(lc, LOG_DEBUG, "%s: %s", key, val);
    }
    LogConfigUnindent(lc);


    HttpResponseStatus(context, HTTP_OK);
    HttpResponseHeader(context, "Server", "Telodendria v" TELODENDRIA_VERSION);
    HttpResponseHeader(context, "Content-Type", "application/json");

    HttpSendHeaders(context);

    stream = HttpStream(context);
    fprintf(stream, "{}\n");
    fclose(stream);

    LogConfigUnindent(lc);
}
