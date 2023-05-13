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
#ifndef CYTOPLASM_HTTPSERVER_H
#define CYTOPLASM_HTTPSERVER_H

/***
 * @Nm HttpServer
 * @Nd Extremely simple HTTP server.
 * @Dd December 13 2022
 * @Xr Http HttpClient
 *
 * .Nm
 * builds on the
 * .Xr Http 3
 * API, and provides a very simple, yet very functional API for
 * creating an HTTP server. It aims at being easy to use and minimal,
 * yet also efficient. It uses non-blocking I/O, is fully
 * multi-threaded, and is very configurable. It can be set up in just
 * two function calls and minimal supporting code.
 * .Pp
 * This API should be familar to those that have dealt with the HTTP
 * server libraries of other programming languages, particularly Java.
 * In fact, much of the terminology used in this API came from Java,
 * and you'll notice that the way responses are sent and received very
 * closely resembles Java.
 */

#include <Http.h>

#include <stdio.h>

#include <HashMap.h>
#include <Stream.h>

/**
 * The functions on this API operate on an opaque structure.
 */
typedef struct HttpServer HttpServer;

/**
 * Each request receives a context structure. It is opaque, so the
 * functions defined in this API should be used to fetch data from
 * it. These functions allow the handler to figure out the context of
 * the request, which includes the path requested, any parameters,
 * and the headers and method used to make the request. The context
 * also provides the means by which the handler responds to the
 * request, allowing it to set the status code, headers, and body.
 */
typedef struct HttpServerContext HttpServerContext;

/**
 * The request handler function is executed when an HTTP request is
 * received. It takes a request context, and a pointer as specified
 * in the server configuration.
 */
typedef void (HttpHandler) (HttpServerContext *, void *);

/**
 * The number of arguments to
 * .Fn HttpServerCreate
 * has grown so large that arguments are now stuffed into a
 * configuration structure, which is in turn passed to
 * .Fn HttpServerCreate .
 * This configuration is copied by value into the internal
 * structures of the server. It is copied with very minimal
 * validation, so ensure that all values are sensible. It may
 * make sense to use
 * .Fn memset
 * to zero out everything in here before assigning values.
 */
typedef struct HttpServerConfig
{
    unsigned short port;
    unsigned int threads;
    unsigned int maxConnections;

    int flags;     /* Http(3) flags */
    char *tlsCert; /* File path */
    char *tlsKey;  /* File path */

    HttpHandler *handler;
    void *handlerArgs;
} HttpServerConfig;

/**
 * Create a new HTTP server using the specified configuration.
 * This will set up all internal structures used by the server,
 * and bind the socket and start listening for connections. However,
 * it will not start accepting connections.
 */
extern HttpServer * HttpServerCreate(HttpServerConfig *);

/**
 * Retrieve the configuration that was used to instantiate the given
 * server. Note that this configuration is not necessarily the exact
 * one that was provided; even though its values are the same, it
 * should be treated as an entirely separate configuration with no
 * connection to the original.
 */
extern HttpServerConfig * HttpServerConfigGet(HttpServer *);

/**
 * Free the resources associated with the given HTTP server. Note that
 * the server can only be freed after it has been stopped. Calling this
 * function while the server is still running results in undefined
 * behavior.
 */
extern void HttpServerFree(HttpServer *);

/**
 * Attempt to start the HTTP server, and return immediately with the
 * status. This API is fully multi-threaded and asynchronous, so the
 * caller can continue working while the HTTP server is running in a
 * separate thread and managing a pool of threads to handle responses.
 */
extern int HttpServerStart(HttpServer *);

/**
 * Typically, at some point after calling
 * .Fn HttpServerStart ,
 * the program will have no more work to do, so it will want to wait
 * for the HTTP server to finish. This is accomplished via this
 * function, which joins the HTTP worker thread to the calling thread,
 * pausing the calling thread until the HTTP server has stopped.
 */
extern void HttpServerJoin(HttpServer *);

/**
 * Stop the HTTP server. Only the execution of this function will
 * cause the proper shutdown of the HTTP server. If the main program
 * is joined to the HTTP thread, then either another thread or a
 * signal handler will have to stop the server using this function.
 * The typical use case is to install a signal handler that executes
 * this function on a global HTTP server.
 */
extern void HttpServerStop(HttpServer *);

/**
 * Get the request headers for the request represented by the given
 * context. The data in the returned hash map should be treated as
 * read only and should not be freed; it is managed entirely by the
 * server.
 */
extern HashMap * HttpRequestHeaders(HttpServerContext *);

/**
 * Get the request method used to make the request represented by
 * the given context.
 */
extern HttpRequestMethod HttpRequestMethodGet(HttpServerContext *);

/**
 * Get the request path for the request represented by the given
 * context. The return value of this function should be treated as
 * read-only, and should not be freed; it is managed entirely by the
 * server.
 */
extern char * HttpRequestPath(HttpServerContext *);

/**
 * Retrieve the parsed GET parameters for the request represented by
 * the given context. The returned hash map should be treated as
 * read-only, and should not be freed; it is managed entirely by the
 * server.
 */
extern HashMap * HttpRequestParams(HttpServerContext *);

/**
 * Set a response header to return to the client. The old value for
 * the given header is returned, if any, otherwise NULL is returned.
 */
extern char * HttpResponseHeader(HttpServerContext *, char *, char *);

/**
 * Set the response status to return to the client.
 */
extern void HttpResponseStatus(HttpServerContext *, HttpStatus);

/**
 * Get the current response status that will be sent to the client
 * making the request represented by the given context.
 */
extern HttpStatus HttpResponseStatusGet(HttpServerContext *);

/**
 * Send the response headers to the client that made the request
 * represented by the specified context. This function must be called
 * before the response body can be written, otherwise a malformed
 * response will be sent.
 */
extern void HttpSendHeaders(HttpServerContext *);

/**
 * Get a stream that is both readable and writable. Reading from the
 * stream reads the request body that the client sent, if there is one.
 * Note that the rquest headers have already been read, so the stream
 * is correctly positioned at the beginning of the body of the request.
 * .Fn HttpSendHeaders
 * must be called before the stream is written, otherwise a malformed
 * HTTP response will be sent. An HTTP handler should properly set all
 * the headers it itends to send, send those headers, and then write
 * the response body to this stream.
 * .Pp
 * Note that the stream does not need to be closed by the HTTP
 * handler; in fact doing so results in undefined behavior. The stream
 * is managed entirely by the server itself, so it will close it when
 * necessary. This allows the underlying protocol to differ: for
 * instance, an HTTP/1.1 connection may stay for multiple requests and
 * responses.
 */
extern Stream * HttpServerStream(HttpServerContext *);

#endif /* CYTOPLASM_HTTPSERVER_H */
