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

#include <Memory.h>
#include <Str.h>
#include <HashMap.h>
#include <HttpClient.h>
#include <Uri.h>
#include <Util.h>

#define FLAG_HEADERS (1 << 0)

static void
usage(char *prog)
{
    StreamPrintf(StreamStderr(), "Usage: %s [-i -X method -H header -d data] url\n", prog);
}

int
main(int argc, char **argv)
{
    HttpClientContext *cx;
    HttpStatus res;
    HttpRequestMethod method = HTTP_GET;
    Uri *uri;
    char *data = NULL;

    HashMap *requestHeaders = HashMapCreate();
    char *key;
    char *val;

    int flags = 0;
    int requestFlags = HTTP_FLAG_NONE;

    int ch;

    while ((ch = getopt(argc, argv, "iH:X:d:")) != -1)
    {
        switch (ch)
        {
            case 'i':
                flags |= FLAG_HEADERS;
                break;
            case 'X':
                method = HttpRequestMethodFromString(optarg);
                if (!method)
                {
                    StreamPrintf(StreamStderr(), "Unknown request method: %s\n", optarg);
                    return 1;
                }
                break;
            case 'H':
                key = optarg;
                val = optarg;

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
                data = optarg;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (argc - optind < 1)
    {
        usage(argv[0]);
        return 1;
    }

    uri = UriParse(argv[optind]);
    if (!uri)
    {
        StreamPrintf(StreamStderr(), "Failed to parse URI: %s\n", argv[optind]);
        return 1;
    }

    if (!uri->port)
    {
        if (strcmp(uri->proto, "https") == 0)
        {
            uri->port = 443;
        }
        else if (strcmp(uri->proto, "http") == 0)
        {
            uri->port = 80;
        }
    }

    if (!uri->port)
    {
        StreamPrintf(StreamStderr(), "Unknown protocol: %s\n", uri->proto);
        UriFree(uri);
        return 1;
    }

    if (strcmp(uri->proto, "https") == 0)
    {
        requestFlags |= HTTP_FLAG_TLS;
    }

    cx = HttpRequest(method, requestFlags, uri->port, uri->host, uri->path);

    if (!cx)
    {
        StreamPuts(StreamStderr(), "Failed to connect.\n");
        UriFree(uri);
        return 1;
    }

    while (HashMapIterate(requestHeaders, &key, (void **) &val))
    {
        HttpRequestHeader(cx, key, val);
        Free(val);
    }

    HttpRequestSendHeaders(cx);
    HashMapFree(requestHeaders);

    if (data)
    {
        if (*data == '@')
        {
            Stream *in;

            data++;

            if (strcmp(data, "-") == 0)
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
                return 1;
            }

            UtilStreamCopy(in, HttpClientStream(cx));

            StreamClose(in);
        }
        else
        {
            StreamPuts(HttpClientStream(cx), data);
        }
    }

    res = HttpRequestSend(cx);

    if (!res)
    {
        StreamPuts(StreamStderr(), "Failed to send request.\n");
        HttpClientContextFree(cx);
        UriFree(uri);
        return 1;
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

    UtilStreamCopy(HttpClientStream(cx), StreamStdout());

    HttpClientContextFree(cx);
    UriFree(uri);

    StreamClose(StreamStdout());
    StreamClose(StreamStderr());
    StreamClose(StreamStdin());

    return !(res == HTTP_OK);
}
