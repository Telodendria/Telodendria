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
#include <stdio.h>
#include <signal.h>

#include <HttpServer.h>
#include <Util.h>

static HttpServer *server = NULL;

static void
SignalHandle(int signal)
{
    (void) signal;
    HttpServerStop(server);
}

static void
HttpHandle(HttpServerContext * cx, void *args)
{
    HashMap *headers = HttpRequestHeaders(cx);
    HttpRequestMethod method = HttpRequestMethodGet(cx);

    char *key;
    char *val;

    size_t bytes;

    printf("%s %s\n", HttpRequestMethodToString(method),
           HttpRequestPath(cx));

    while (HashMapIterate(headers, &key, (void **) &val))
    {
        printf("%s: %s\n", key, val);
    }

    printf("\n");

    bytes = UtilStreamCopy(HttpStream(cx), stdout);

    printf("\n");
    printf("(%lu bytes)\n", bytes);

    HttpSendHeaders(cx);

    fprintf(HttpStream(cx), "{}\n");
}

int
main(void)
{
    struct sigaction sa;

    server = HttpServerCreate(8008, 1, 1, HttpHandle, NULL);
    if (!HttpServerStart(server))
    {
        fprintf(stderr, "Unable to start HTTP server.\n");
        HttpServerFree(server);
        return 1;
    }

    printf("Listening on port 8008.\n");

    sa.sa_handler = SignalHandle;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        fprintf(stderr, "Unable to install signal handler.\n");
        HttpServerStop(server);
        HttpServerJoin(server);
        HttpServerFree(server);
        return 1;
    }

    HttpServerJoin(server);

    printf("Shutting down.\n");
    HttpServerStop(server);
    return 0;
}
