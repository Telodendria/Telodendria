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
#include <Http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Memory.h>
#include <HashMap.h>
#include <Util.h>
#include <Str.h>

#ifndef CYTOPLASM_STRING_CHUNK
#define CYTOPLASM_STRING_CHUNK 64
#endif

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
    if (StrEquals(str, "GET"))
    {
        return HTTP_GET;
    }

    if (StrEquals(str, "HEAD"))
    {
        return HTTP_HEAD;
    }

    if (StrEquals(str, "POST"))
    {
        return HTTP_POST;
    }

    if (StrEquals(str, "PUT"))
    {
        return HTTP_PUT;
    }

    if (StrEquals(str, "DELETE"))
    {
        return HTTP_DELETE;
    }

    if (StrEquals(str, "CONNECT"))
    {
        return HTTP_CONNECT;
    }

    if (StrEquals(str, "OPTIONS"))
    {
        return HTTP_OPTIONS;
    }

    if (StrEquals(str, "TRACE"))
    {
        return HTTP_TRACE;
    }

    if (StrEquals(str, "PATCH"))
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

    size = CYTOPLASM_STRING_CHUNK;
    len = 0;
    encoded = Malloc(size);
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

            size += CYTOPLASM_STRING_CHUNK;
            tmp = Realloc(encoded, size);
            if (!tmp)
            {
                Free(encoded);
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
    decoded = Malloc(inputLen + 1);

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
                Free(decoded);
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
HttpParamDecode(char *in)
{
    HashMap *params;

    if (!in)
    {
        return NULL;
    }

    params = HashMapCreate();
    if (!params)
    {
        return NULL;
    }

    while (*in)
    {
        char *buf;
        size_t allocated;
        size_t len;

        char *decKey;
        char *decVal;

        /* Read in key */

        allocated = CYTOPLASM_STRING_CHUNK;
        buf = Malloc(allocated);
        len = 0;

        while (*in && *in != '=')
        {
            if (len >= allocated - 1)
            {
                allocated += CYTOPLASM_STRING_CHUNK;
                buf = Realloc(buf, allocated);
            }

            buf[len] = *in;
            len++;
            in++;
        }

        buf[len] = '\0';

        /* Sanity check */
        if (*in != '=')
        {
            /* Malformed param */
            Free(buf);
            HashMapFree(params);
            return NULL;
        }

        in++;

        /* Decode key */
        decKey = HttpUrlDecode(buf);
        Free(buf);

        if (!decKey)
        {
            /* Decoding error */
            HashMapFree(params);
            return NULL;
        }

        /* Read in value */
        allocated = CYTOPLASM_STRING_CHUNK;
        buf = Malloc(allocated);
        len = 0;

        while (*in && *in != '&')
        {
            if (len >= allocated - 1)
            {
                allocated += CYTOPLASM_STRING_CHUNK;
                buf = Realloc(buf, allocated);
            }

            buf[len] = *in;
            len++;
            in++;
        }

        buf[len] = '\0';

        /* Decode value */
        decVal = HttpUrlDecode(buf);
        Free(buf);

        if (!decVal)
        {
            /* Decoding error */
            HashMapFree(params);
            return NULL;
        }

        buf = HashMapSet(params, decKey, decVal);
        if (buf)
        {
            Free(buf);
        }
        Free(decKey);

        if (*in == '&')
        {
            in++;
            continue;
        }
        else
        {
            break;
        }
    }

    return params;
}

char *
HttpParamEncode(HashMap * params)
{
    char *key;
    char *val;
    char *out = NULL;

    if (!params || !out)
    {
        return NULL;
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
            Free(encKey);
            Free(encVal);
            return NULL;
        }

        /* TODO */

        Free(encKey);
        Free(encVal);
    }

    return out;
}

HashMap *
HttpParseHeaders(Stream * fp)
{
    HashMap *headers;

    char *line;
    ssize_t lineLen;
    size_t lineSize;

    char *headerKey;
    char *headerValue;

    if (!fp)
    {
        return NULL;
    }


    headers = HashMapCreate();
    if (!headers)
    {
        return NULL;
    }

    line = NULL;
    lineLen = 0;

    while ((lineLen = UtilGetLine(&line, &lineSize, fp)) != -1)
    {
        char *headerPtr;

        ssize_t i;
        size_t len;

        if (StrEquals(line, "\r\n") || StrEquals(line, "\n"))
        {
            break;
        }

        for (i = 0; i < lineLen; i++)
        {
            if (line[i] == ':')
            {
                line[i] = '\0';
                break;
            }

            line[i] = tolower((unsigned char) line[i]);
        }

        len = i + 1;
        headerKey = Malloc(len * sizeof(char));
        if (!headerKey)
        {
            goto error;
        }

        strncpy(headerKey, line, len);

        headerPtr = line + i + 1;

        while (isspace((unsigned char) *headerPtr))
        {
            headerPtr++;
        }

        for (i = lineLen - 1; i > (line + lineLen) - headerPtr; i--)
        {
            if (!isspace((unsigned char) line[i]))
            {
                break;
            }
            line[i] = '\0';
        }

        len = strlen(headerPtr) + 1;
        headerValue = Malloc(len * sizeof(char));
        if (!headerValue)
        {
            Free(headerKey);
            goto error;
        }

        strncpy(headerValue, headerPtr, len);

        HashMapSet(headers, headerKey, headerValue);
        Free(headerKey);
    }

    Free(line);
    return headers;

error:
    Free(line);

    while (HashMapIterate(headers, &headerKey, (void **) &headerValue))
    {
        Free(headerValue);
    }

    HashMapFree(headers);

    return NULL;
}
