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
#include <Http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Constants.h>
#include <HashMap.h>

const char *
HttpRequestMethodToString(const HttpRequestMethod method)
{
    switch (method)
    {
            case HTTP_GET:
            return "GET";
        case HTTP_HEAD:
            return "HEAD";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_CONNECT:
            return "CONNECT";
        case HTTP_OPTIONS:
            return "OPTIONS";
        case HTTP_TRACE:
            return "TRACE";
        case HTTP_PATCH:
            return "PATCH";
        default:
            return NULL;
    }
}

HttpRequestMethod
HttpRequestMethodFromString(const char *str)
{
    if (strcmp(str, "GET") == 0)
    {
        return HTTP_GET;
    }

    if (strcmp(str, "HEAD") == 0)
    {
        return HTTP_HEAD;
    }

    if (strcmp(str, "POST") == 0)
    {
        return HTTP_POST;
    }

    if (strcmp(str, "PUT") == 0)
    {
        return HTTP_PUT;
    }

    if (strcmp(str, "DELETE") == 0)
    {
        return HTTP_DELETE;
    }

    if (strcmp(str, "CONNECT") == 0)
    {
        return HTTP_CONNECT;
    }

    if (strcmp(str, "OPTIONS") == 0)
    {
        return HTTP_OPTIONS;
    }

    if (strcmp(str, "TRACE") == 0)
    {
        return HTTP_TRACE;
    }

    if (strcmp(str, "PATCH") == 0)
    {
        return HTTP_PATCH;
    }

    return HTTP_METHOD_UNKNOWN;
}

const char *
HttpStatusToString(const HttpStatus status)
{
    switch (status)
    {
            case HTTP_CONTINUE:
            return "Continue";
        case HTTP_SWITCHING_PROTOCOLS:
            return "Switching Protocols";
        case HTTP_EARLY_HINTS:
            return "Early Hints";
        case HTTP_OK:
            return "Ok";
        case HTTP_CREATED:
            return "Created";
        case HTTP_ACCEPTED:
            return "Accepted";
        case HTTP_NON_AUTHORITATIVE_INFORMATION:
            return "Non-Authoritative Information";
        case HTTP_NO_CONTENT:
            return "No Content";
        case HTTP_RESET_CONTENT:
            return "Reset Content";
        case HTTP_PARTIAL_CONTENT:
            return "Partial Content";
        case HTTP_MULTIPLE_CHOICES:
            return "Multiple Choices";
        case HTTP_MOVED_PERMANENTLY:
            return "Moved Permanently";
        case HTTP_FOUND:
            return "Found";
        case HTTP_SEE_OTHER:
            return "See Other";
        case HTTP_NOT_MODIFIED:
            return "Not Modified";
        case HTTP_TEMPORARY_REDIRECT:
            return "Temporary Redirect";
        case HTTP_PERMANENT_REDIRECT:
            return "Permanent Redirect";
        case HTTP_BAD_REQUEST:
            return "Bad Request";
        case HTTP_UNAUTHORIZED:
            return "Unauthorized";
        case HTTP_FORBIDDEN:
            return "Forbidden";
        case HTTP_NOT_FOUND:
            return "Not Found";
        case HTTP_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP_NOT_ACCEPTABLE:
            return "Not Acceptable";
        case HTTP_PROXY_AUTH_REQUIRED:
            return "Proxy Authentication Required";
        case HTTP_REQUEST_TIMEOUT:
            return "Request Timeout";
        case HTTP_CONFLICT:
            return "Conflict";
        case HTTP_GONE:
            return "Gone";
        case HTTP_LENGTH_REQUIRED:
            return "Length Required";
        case HTTP_PRECONDITION_FAILED:
            return "Precondition Failed";
        case HTTP_PAYLOAD_TOO_LARGE:
            return "Payload Too Large";
        case HTTP_URI_TOO_LONG:
            return "URI Too Long";
        case HTTP_UNSUPPORTED_MEDIA_TYPE:
            return "Unsupported Media Type";
        case HTTP_RANGE_NOT_SATISFIABLE:
            return "Range Not Satisfiable";
        case HTTP_EXPECTATION_FAILED:
            return "Expectation Failed";
        case HTTP_TEAPOT:
            return "I'm a Teapot";
        case HTTP_UPGRADE_REQUIRED:
            return "Upgrade Required";
        case HTTP_PRECONDITION_REQUIRED:
            return "Precondition Required";
        case HTTP_TOO_MANY_REQUESTS:
            return "Too Many Requests";
        case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE:
            return "Request Header Fields Too Large";
        case HTTP_UNAVAILABLE_FOR_LEGAL_REASONS:
            return "Unavailable For Legal Reasons";
        case HTTP_INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case HTTP_NOT_IMPLEMENTED:
            return "Not Implemented";
        case HTTP_BAD_GATEWAY:
            return "Bad Gateway";
        case HTTP_SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        case HTTP_GATEWAY_TIMEOUT:
            return "Gateway Timeout";
        case HTTP_VERSION_NOT_SUPPORTED:
            return "Version Not Supported";
        case HTTP_VARIANT_ALSO_NEGOTIATES:
            return "Variant Also Negotiates";
        case HTTP_NOT_EXTENDED:
            return "Not Extended";
        case HTTP_NETWORK_AUTH_REQUIRED:
            return "Network Authentication Required";
        default:
            return NULL;
    }
}

char *
HttpUrlEncode(char *str)
{
    size_t size;
    size_t len;
    char *encoded;

    if (!str)
    {
        return NULL;
    }

    size = TELODENDRIA_STRING_CHUNK;
    len = 0;
    encoded = malloc(size);
    if (!encoded)
    {
        return NULL;
    }

    while (*str)
    {
        char c = *str;

        if (len >= size - 4)
        {
            char *tmp;

            size += TELODENDRIA_STRING_CHUNK;
            tmp = realloc(encoded, size);
            if (!tmp)
            {
                free(encoded);
                return NULL;
            }

            encoded = tmp;
        }

        /* Control characters and extended characters */
        if (c <= 0x1F || c >= 0x7F)
        {
            goto percentEncode;
        }

        /* Reserved and unsafe characters */
        switch (c)
        {
            case '$':
            case '&':
            case '+':
            case ',':
            case '/':
            case ':':
            case ';':
            case '=':
            case '?':
            case '@':
            case ' ':
            case '"':
            case '<':
            case '>':
            case '#':
            case '%':
            case '{':
            case '}':
            case '|':
            case '\\':
            case '^':
            case '~':
            case '[':
            case ']':
            case '`':
                goto percentEncode;
                break;
            default:
                encoded[len] = c;
                len++;
                str++;
                continue;
        }

percentEncode:
        encoded[len] = '%';
        len++;
        snprintf(encoded + len, 3, "%2X", c);
        len += 2;

        str++;
    }

    encoded[len] = '\0';
    return encoded;
}

char *
HttpUrlDecode(char *str)
{
    size_t i;
    size_t inputLen;
    char *decoded;

    if (!str)
    {
        return NULL;
    }

    i = 0;
    inputLen = strlen(str);
    decoded = malloc(inputLen);

    if (!decoded)
    {
        return NULL;
    }

    while (*str)
    {
        char c = *str;

        if (c == '%')
        {
            unsigned int d;

            str++;

            if (sscanf(str, "%2X", &d) != 1)
            {
                /* Decoding error */
                free(decoded);
                return NULL;
            }

            if (!d)
            {
                /* Null character given, don't put that in the string. */
                continue;
            }

            c = (char) d;

            str++;
        }

        decoded[i] = c;
        i++;

        str++;
    }

    decoded[i] = '\0';

    return decoded;
}

HashMap *
HttpParamDecode(FILE * in)
{
    /* TODO */
    (void) in;
    return NULL;
}

void
HttpParamEncode(HashMap * params, FILE * out)
{
    char *key;
    char *val;

    if (!params || !out)
    {
        return;
    }

    while (HashMapIterate(params, &key, (void *) &val))
    {
        char *encKey;
        char *encVal;

        encKey = HttpUrlEncode(key);
        encVal = HttpUrlEncode(val);

        if (!encKey || !encVal)
        {
            /* Memory error */
            return;
        }

        fprintf(out, "%s=%s&", encKey, encVal);

        free(encKey);
        free(encVal);
    }
}
