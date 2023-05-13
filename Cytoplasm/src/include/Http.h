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
#ifndef CYTOPLASM_HTTP_H
#define CYTOPLASM_HTTP_H

/***
 * @Nm Http
 * @Nd Encode and decode various parts of the HTTP protocol.
 * @Dd March 12 2023
 * @Xr HttpClient HttpServer HashMap Queue Memory
 *
 * .Nm
 * is a collection of utility functions and type definitions that are
 * useful for dealing with HTTP. HTTP is not a complex protocol, but
 * this API makes it a lot easier to work with.
 * .Pp
 * Note that this API doesn't target any particular HTTP version, but
 * it is currently used with HTTP 1.0 clients and servers, and
 * therefore may be lacking functionality added in later HTTP versions.
 */

#include <stdio.h>

#include <HashMap.h>
#include <Stream.h>

#define HTTP_FLAG_NONE 0
#define HTTP_FLAG_TLS (1 << 0)

/**
 * The request methods defined by the HTTP standard. These numeric
 * constants should be preferred to strings when building HTTP APIs
 * because they are more efficient.
 */
typedef enum HttpRequestMethod
{
    HTTP_METHOD_UNKNOWN,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_CONNECT,
    HTTP_OPTIONS,
    HTTP_TRACE,
    HTTP_PATCH
} HttpRequestMethod;

/**
 * An enumeration that corresponds to the actual integer values of the
 * valid HTTP response codes.
 */
typedef enum HttpStatus
{
    HTTP_STATUS_UNKNOWN = 0,

    /* Informational responses */
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_EARLY_HINTS = 103,

    /* Successful responses */
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,

    /* Redirection messages */
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_PERMANENT_REDIRECT = 308,

    /* Client error messages */
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTH_REQUIRED = 407,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_PAYLOAD_TOO_LARGE = 413,
    HTTP_URI_TOO_LONG = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_RANGE_NOT_SATISFIABLE = 416,
    HTTP_EXPECTATION_FAILED = 417,
    HTTP_TEAPOT = 418,
    HTTP_UPGRADE_REQUIRED = 426,
    HTTP_PRECONDITION_REQUIRED = 428,
    HTTP_TOO_MANY_REQUESTS = 429,
    HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

    /* Server error responses */
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_NOT_EXTENDED = 510,
    HTTP_NETWORK_AUTH_REQUIRED = 511
} HttpStatus;

/**
 * Convert an HTTP status enumeration value into a string description
 * of the status, which is to be used in server response to a client,
 * or a client response to a user. For example, calling
 * .Fn HttpStatusToString "HTTP_GATEWAY_TIMEOUT"
 * (or
 * .Fn HttpStatusToString "504" )
 * produces the string "Gateway Timeout". Note that the returned
 * pointers point to static space, so their manipulation is forbidden.
 */
extern const char * HttpStatusToString(const HttpStatus);

/**
 * Convert a string into a numeric code that can be used throughout
 * the code of a program in an efficient manner. See the definition
 * of HttpRequestMethod. This function does case-sensitive matching,
 * and does not trim or otherwise process the input string.
 */
extern HttpRequestMethod HttpRequestMethodFromString(const char *);

/**
 * Convert a numeric code as defined by HttpRequestMethod into a
 * string that can be sent to a server. Note that the returned pointers
 * point to static space, so their manipulation is forbidden.
 */
extern const char * HttpRequestMethodToString(const HttpRequestMethod);

/**
 * Encode a C string such that it can safely appear in a URL by
 * performing the necessary percent escaping. A new string on the
 * heap is returned. It should be freed with
 * .Fn Free ,
 * defined in the
 * .Xr Memory 3
 * API.
 */
extern char * HttpUrlEncode(char *);

/**
 * Decode a percent-encoded string into a C string, ignoring encoded
 * null characters entirely, because those would do nothing but cause
 * problems.
 */
extern char * HttpUrlDecode(char *);

/**
 * Decode an encoded parameter string in the form of
 * ``key=val&key2=val2'' into a hash map whose values are C strings.
 * This function properly decodes keys and values using the functions
 * defined above.
 */
extern HashMap * HttpParamDecode(char *);

/**
 * Encode a hash map whose values are strings as an HTTP parameter
 * string suitable for GET or POST requests.
 */
extern char * HttpParamEncode(HashMap *);

/**
 * Read HTTP headers from a stream and return a hash map whose values
 * are strings. All keys are lowercased to make querying them
 * consistent and not dependent on the case that was read from the
 * stream. This is useful for both client and server code, since the
 * headers are in the same format. This function should be used after
 * parsing the HTTP status line, because it does not parse that line.
 * It will stop when it encounters the first blank line, which
 * indicates that the body is beginning. After this function completes,
 * the body may be immediately read from the stream without any
 * additional processing.
 */
extern HashMap * HttpParseHeaders(Stream *);

#endif
