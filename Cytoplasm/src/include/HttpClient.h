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
#ifndef CYTOPLASM_HTTPCLIENT_H
#define CYTOPLASM_HTTPCLIENT_H

/***
 * @Nm HttpClient
 * @Nd Extremely simple HTTP client.
 * @Dd April 29 2023
 * @Xr Http HttpServer Tls
 * 
 * .Nm
 * HttpClient
 * builds on the HTTP API to provide a simple yet functional HTTP
 * client. It aims at being easy to use and minimal, yet also
 * efficient.
 */

#include <stdio.h>

#include <HashMap.h>
#include <Http.h>

/**
 * A server response is represented by a client context. It is
 * opaque, so the functions defined in this API should be used to
 * fetch data from and manipulate it.
 */
typedef struct HttpClientContext HttpClientContext;

/**
 * Make an HTTP request. This function takes the request method,
 * any flags defined in the HTTP API, the port, hostname, and path,
 * all in that order. It returns NULL if there was an error making
 * the request. Otherwise it returns a client context. Note that this
 * function does not actually send any data, it simply makes the
 * connection. Use
 * .Fn HttpRequestHeader
 * to add headers to the request. Then, send headers with
 * .Fn HttpRequestSendHeaders .
 * Finally, the request body, if any, can be written to the output
 * stream, and then the request can be fully sent using
 * .Fn HttpRequestSend .
 */
extern HttpClientContext *
 HttpRequest(HttpRequestMethod, int, unsigned short, char *, char *);

/**
 * Set a request header to send to the server when making the
 * request.
 */
extern void HttpRequestHeader(HttpClientContext *, char *, char *);

/**
 * Send the request headers to the server. This must be called before
 * the request body can be written or a response body can be read.
 */
extern void HttpRequestSendHeaders(HttpClientContext *);

/**
 * Flush the request stream to the server. This function should be
 * called before the response body is read.
 */
extern HttpStatus HttpRequestSend(HttpClientContext *);

/**
 * Get the headers returned by the server.
 */
extern HashMap * HttpResponseHeaders(HttpClientContext *);

/**
 * Get the stream used to write the request body and read the
 * response body.
 */
extern Stream * HttpClientStream(HttpClientContext *);

/**
 * Free all memory associated with the client context. This closes the
 * connection, if it was still open.
 */
extern void HttpClientContextFree(HttpClientContext *);

#endif                             /* CYTOPLASM_HTTPCLIENT_H */
