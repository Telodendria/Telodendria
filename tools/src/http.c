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
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <Args.h>
#include <Memory.h>
#include <Str.h>
#include <HashMap.h>
#include <HttpClient.h>
#include <Uri.h>

#define FLAG_HEADERS (1 << 0)

static void
usage(char *prog)
{
    StreamPrintf(StreamStderr(), "Usage: %s [-i -X method -H header -d data] url\n", prog);
}

int
Main(Array * args)
{
    HttpClientContext *cx = NULL;
    HttpStatus res;
    HttpRequestMethod method = HTTP_GET;
    Uri *uri = NULL;
    char *data = NULL;

    HashMap *requestHeaders = HashMapCreate();
    char *key;
    char *val;

    ArgParseState arg;
    int flags = 0;
    int requestFlags = HTTP_FLAG_NONE;

    int ch;
    int ret = 1;

    ArgParseStateInit(&arg);
    while ((ch = ArgParse(&arg, args, "iH:X:d:")) != -1)
    {
        switch (ch)
        {
            case 'i':
                flags |= FLAG_HEADERS;
                break;
            case 'X':
                method = HttpRequestMethodFromString(arg.optArg);
                if (!method)
                {
                    StreamPrintf(StreamStderr(), "Unknown request method: %s\n", arg.optArg);
                    return 1;
                }
                break;
            case 'H':
                key = arg.optArg;
                val = arg.optArg;

                while (*val && *val != ':')
                {
                    val++;
                }

                *val = '\0';
                val++;

                while (*val && isspace((unsigned char) *val))
                {
                    val++;
                }

                HashMapSet(requestHeaders, key, StrDuplicate(val));
                break;
            case 'd':
                data = arg.optArg;
                break;
            default:
                usage(ArrayGet(args, 0));
                goto finish;
        }
    }

    if (ArraySize(args) - arg.optInd < 1)
    {
        usage(ArrayGet(args, 0));
        goto finish;
    }

    uri = UriParse(ArrayGet(args, arg.optInd));
    if (!uri)
    {
        StreamPrintf(StreamStderr(), "Failed to parse URI: %s\n", ArrayGet(args, arg.optInd));
        goto finish;
    }

    if (!uri->port)
    {
        if (StrEquals(uri->proto, "https"))
        {
            uri->port = 443;
        }
        else if (StrEquals(uri->proto, "http"))
        {
            uri->port = 80;
        }
    }

    if (!uri->port)
    {
        StreamPrintf(StreamStderr(), "Unknown protocol: %s\n", uri->proto);
        goto finish;
    }

    if (StrEquals(uri->proto, "https"))
    {
        requestFlags |= HTTP_FLAG_TLS;
    }

    cx = HttpRequest(method, requestFlags, uri->port, uri->host, uri->path);

    if (!cx)
    {
        StreamPuts(StreamStderr(), "Failed to connect.\n");
        goto finish;
    }

    while (HashMapIterate(requestHeaders, &key, (void **) &val))
    {
        HttpRequestHeader(cx, key, val);
        Free(val);
    }

    if (data)
    {
        if (*data == '@')
        {
            Stream *in;
            int len;

            data++;

            if (StrEquals(data, "-"))
            {
                in = StreamStdin();
            }
            else
            {
                in = StreamOpen(data, "r");
            }

            if (!in)
            {
                StreamPrintf(StreamStderr(), "%s: %s\n", data, strerror(errno));
                goto finish;
            }

            len = StreamSeek(in, 0, SEEK_END);
            if (len > -1)
            {
                char *lenStr;
                int nBytes;

                StreamSeek(in, 0, SEEK_SET);
                nBytes = snprintf(NULL, 0, "%d", len);

                lenStr = Malloc(nBytes + 1);
                snprintf(lenStr, nBytes + 1, "%d", len);

                HttpRequestHeader(cx, "Content-Length", lenStr);
                Free(lenStr);
            }

            HttpRequestSendHeaders(cx);
            StreamCopy(in, HttpClientStream(cx));
            if (in != StreamStdin())
            {
                StreamClose(in);
            }
        }
        else
        {
            char *lenStr;
            int len = strlen(data);
            int nBytes = snprintf(NULL, 0, "%d", len);

            lenStr = Malloc(nBytes + 1);
            snprintf(lenStr, nBytes + 1, "%d", len);

            HttpRequestHeader(cx, "Content-Length", lenStr);
            Free(lenStr);
            HttpRequestSendHeaders(cx);
            StreamPuts(HttpClientStream(cx), data);
        }
    }
    else
    {
        HttpRequestSendHeaders(cx);
    }

    res = HttpRequestSend(cx);

    if (!res)
    {
        StreamPuts(StreamStderr(), "Failed to send request.\n");
        goto finish;
    }

    if (flags & FLAG_HEADERS)
    {
        HashMap *responseHeaders = HttpResponseHeaders(cx);

        StreamPrintf(StreamStdout(), "HTTP/1.0 %d %s\n", res, HttpStatusToString(res));

        while (HashMapIterate(responseHeaders, &key, (void **) &val))
        {
            StreamPrintf(StreamStdout(), "%s: %s\n", key, val);
        }

        StreamPutc(StreamStdout(), '\n');
    }

    StreamCopy(HttpClientStream(cx), StreamStdout());

    ret = !(res == HTTP_OK);

finish:
    HashMapFree(requestHeaders);

    HttpClientContextFree(cx);
    UriFree(uri);

    return ret;
}
