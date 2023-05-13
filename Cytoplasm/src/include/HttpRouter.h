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
#ifndef CYTOPLASM_HTTPROUTER_H
#define CYTOPLASM_HTTPROUTER_H

/***
 * @Nm HttpRouter
 * @Nd Simple HTTP request router with regular expression support.
 * @Dd April 29 2023
 * @Xr HttpServer Http
 *
 * .Nm
 * provides a simple mechanism for assigning functions to an HTTP
 * request path. It is a simple tree data structure that parses the
 * registered request paths and maps functions onto each part of the
 * path. Then, requests can be easily routed to their appropriate
 * handler functions.
 */

#include <Array.h>

/**
 * The router structure is opaque and thus managed entirely  by the
 * functions defined in this API.
 */
typedef struct HttpRouter HttpRouter;

/**
 * A function written to handle an HTTP request takes an array
 * consisting of the matched path parts in the order they appear in
 * the path, and a pointer to caller-provided arguments, if any.
 * It returns a pointer that the caller is assumed to know how to
 * handle.
 */
typedef void *(HttpRouteFunc) (Array *, void *);

/**
 * Create a new empty routing tree.
 */
extern HttpRouter * HttpRouterCreate(void);

/**
 * Free all the memory associated with the given routing tree.
 */
extern void HttpRouterFree(HttpRouter *);

/**
 * Register the specified route function to be executed upon requests
 * for the specified HTTP path. The path is parsed by splitting at
 * each path separator. Each part of the path is a regular expression
 * that matches the entire path part. A regular expression cannot
 * match more than one path part. This allows for paths like
 * .Pa /some/path/(.*)/parts
 * to work as one would expect.
 */
extern int HttpRouterAdd(HttpRouter *, char *, HttpRouteFunc *);

/**
 * Route the specified request path using the specified routing
 * tree. This function will parse the path and match it to the 
 * appropriate route handler function. The return value is a boolean
 * value that indicates whether or not an appropriate route function
 * was found. If an appropriate function was found, then the void
 * pointer is passed to it as arguments that it is expected to know
 * how to handle, and the pointer to a void pointer is where the
 * route function's response will be placed.
 */
extern int HttpRouterRoute(HttpRouter *, char *, void *, void **);

#endif                             /* CYTOPLASM_HTTPROUTER_H */
