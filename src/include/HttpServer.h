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
#ifndef TELODENDRIA_HTTPSERVER_H
#define TELODENDRIA_HTTPSERVER_H

#include <Http.h>

#include <HashMap.h>

typedef struct HttpServer HttpServer;

extern HttpServer *
 HttpServerCreate(unsigned short, unsigned int, unsigned int, HttpHandler *, void *);

extern void
 HttpServerFree(HttpServer *);

extern int
 HttpServerStart(HttpServer *);

extern void
 HttpServerJoin(HttpServer *);

extern void
 HttpServerStop(HttpServer *);

typedef struct HttpServerContext HttpServerContext;
typedef void (HttpHandler) (HttpServerContext *, void *);

extern HashMap *
 HttpRequestHeaders(HttpServerContext *);

extern HttpRequestMethod
 HttpRequestMethod(HttpServerContext *);

extern char *
 HttpRequestPath(HttpServerContext *);

extern void
 HttpResponseHeader(HttpServerContext *, const char *, const char *);

extern void
 HttpResponseStatus(HttpServerContext *, HttpStatus);

extern FILE *
 HttpStream(HttpServerContext *);

extern void
 HttpSendHeaders(HttpServerContext *);

#endif
