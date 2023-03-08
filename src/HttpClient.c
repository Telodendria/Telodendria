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
#include <HttpClient.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <Http.h>
#include <Memory.h>
#include <Util.h>

struct HttpClientContext
{
    HashMap *responseHeaders;
    FILE *stream;
};

HttpClientContext *
HttpRequest(HttpRequestMethod method, int flags, unsigned short port, char *host, char *path)
{
    HttpClientContext *context;

    int sd;
    struct addrinfo hints, *res, *res0;
    int error;

    char serv[8];

    if (!method || !host || !path)
    {
        return NULL;
    }

    if (!port)
    {
        if (flags & HTTP_TLS)
        {
            strcpy(serv, "https");
        }
        else
        {
            strcpy(serv, "www");
        }
    }
    else
    {
        sprintf(serv, "%hu", port);
    }

    /* TODO: Not supported yet */
    if (flags & HTTP_TLS)
    {
        return NULL;
    }

    context = Malloc(sizeof(HttpClientContext));
    if (!context)
    {
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(host, serv, &hints, &res0);

    if (error)
    {
        Free(context);
        return NULL;
    }

    for (res = res0; res; res = res->ai_next)
    {
        sd = socket(res->ai_family, res->ai_socktype,
                    res->ai_protocol);

        if (sd < 0)
        {
            continue;
        }

        if (connect(sd, res->ai_addr, res->ai_addrlen) < 0)
        {
            close(sd);
            sd = -1;
            continue;
        }

        break;
    }

    if (sd < 0)
    {
        Free(context);
        return NULL;
    }

    freeaddrinfo(res0);

    context->stream = fdopen(sd, "r+");
    if (!context->stream)
    {
        Free(context);
        close(sd);
        return NULL;
    }

    fprintf(context->stream, "%s %s HTTP/1.1\r\n",
            HttpRequestMethodToString(method), path);

    HttpRequestHeader(context, "Connection", "close");
    HttpRequestHeader(context, "User-Agent", "Telodendria/" TELODENDRIA_VERSION);
    HttpRequestHeader(context, "Host", host);

    return context;
}

void
HttpRequestHeader(HttpClientContext * context, char *key, char *val)
{
    if (!context || !key || !val)
    {
        return;
    }

    fprintf(context->stream, "%s: %s\r\n", key, val);
}

void
HttpRequestSendHeaders(HttpClientContext * context)
{
    if (!context)
    {
        return;
    }

    fprintf(context->stream, "\r\n");
    fflush(context->stream);
}

HttpStatus
HttpRequestSend(HttpClientContext * context)
{
    HttpStatus status;

    char *line = NULL;
    ssize_t lineLen;
    size_t lineSize = 0;
    char *tmp;

    if (!context)
    {
        return 0;
    }

    lineLen = UtilGetLine(&line, &lineSize, context->stream);

    /* Line must contain at least "HTTP/x.x xxx" */
    if (lineLen < 12)
    {
        return 0;
    }

    if (!(strncmp(line, "HTTP/1.0", 8) == 0 ||
          strncmp(line, "HTTP/1.1", 8) == 0))
    {
        return 0;
    }

    tmp = line + 9;

    while (isspace(*tmp) && *tmp != '\0')
    {
        tmp++;
    }

    if (!*tmp)
    {
        return 0;
    }

    status = atoi(tmp);

    if (!status)
    {
        return 0;
    }

    context->responseHeaders = HttpParseHeaders(context->stream);
    if (!context->responseHeaders)
    {
        return 0;
    }

    return status;
}

HashMap *
HttpResponseHeaders(HttpClientContext * context)
{
    if (!context)
    {
        return NULL;
    }

    return context->responseHeaders;
}

FILE *
HttpClientStream(HttpClientContext * context)
{
    if (!context)
    {
        return NULL;
    }

    return context->stream;
}

void
HttpClientContextFree(HttpClientContext * context)
{
    char *key;
    void *val;

    if (!context)
    {
        return;
    }

    while (HashMapIterate(context->responseHeaders, &key, &val))
    {
        Free(val);
    }

    HashMapFree(context->responseHeaders);

    fclose(context->stream);
    Free(context);
}
