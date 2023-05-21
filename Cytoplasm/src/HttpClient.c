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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <Http.h>
#include <Memory.h>
#include <Util.h>
#include <Tls.h>

struct HttpClientContext
{
    HashMap *responseHeaders;
    Stream *stream;
};

HttpClientContext *
HttpRequest(HttpRequestMethod method, int flags, unsigned short port, char *host, char *path)
{
    HttpClientContext *context;

    int sd = -1;
    struct addrinfo hints, *res, *res0;
    int error;

    char serv[8];

    if (!method || !host || !path)
    {
        return NULL;
    }

#ifndef TLS_IMPL
    if (flags & HTTP_FLAG_TLS)
    {
        return NULL;
    }
#endif

    if (!port)
    {
        if (flags & HTTP_FLAG_TLS)
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
        snprintf(serv, sizeof(serv), "%hu", port);
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

#ifdef TLS_IMPL
    if (flags & HTTP_FLAG_TLS)
    {
        context->stream = TlsClientStream(sd, host);
    }
    else
    {
        context->stream = StreamFd(sd);
    }
#else
    context->stream = StreamFd(sd);
#endif

    if (!context->stream)
    {
        Free(context);
        close(sd);
        return NULL;
    }

    StreamPrintf(context->stream, "%s %s HTTP/1.0\r\n",
                 HttpRequestMethodToString(method), path);

    HttpRequestHeader(context, "Connection", "close");
    HttpRequestHeader(context, "User-Agent", LIB_NAME "/" LIB_VERSION);
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

    StreamPrintf(context->stream, "%s: %s\r\n", key, val);
}

void
HttpRequestSendHeaders(HttpClientContext * context)
{
    if (!context)
    {
        return;
    }

    StreamPuts(context->stream, "\r\n");
    StreamFlush(context->stream);
}

HttpStatus
HttpRequestSend(HttpClientContext * context)
{
    HttpStatus status = HTTP_STATUS_UNKNOWN;

    char *line = NULL;
    ssize_t lineLen;
    size_t lineSize = 0;
    char *tmp;

    if (!context)
    {
        goto finish;
    }

    StreamFlush(context->stream);

    lineLen = UtilGetLine(&line, &lineSize, context->stream);

    while (lineLen == -1 && errno == EAGAIN)
    {
        StreamClearError(context->stream);
        lineLen = UtilGetLine(&line, &lineSize, context->stream);
    }

    if (lineLen == -1)
    {
        goto finish;
    }

    /* Line must contain at least "HTTP/x.x xxx" */
    if (lineLen < 12)
    {
        goto finish;
    }

    if (!(strncmp(line, "HTTP/1.0", 8) == 0 ||
          strncmp(line, "HTTP/1.1", 8) == 0))
    {
        goto finish;
    }

    tmp = line + 9;

    while (isspace((unsigned char) *tmp) && *tmp != '\0')
    {
        tmp++;
    }

    if (!*tmp)
    {
        goto finish;
    }

    status = atoi(tmp);

    if (!status)
    {
        status = HTTP_STATUS_UNKNOWN;
        goto finish;
    }

    context->responseHeaders = HttpParseHeaders(context->stream);
    if (!context->responseHeaders)
    {
        status = HTTP_STATUS_UNKNOWN;
        goto finish;
    }

finish:
    Free(line);
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

Stream *
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

    StreamClose(context->stream);
    Free(context);
}
