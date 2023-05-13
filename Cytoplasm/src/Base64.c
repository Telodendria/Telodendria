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
#include <Base64.h>

#include <Memory.h>

static const char Base64EncodeMap[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const int Base64DecodeMap[] = {
    62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51
};

size_t
Base64EncodedSize(size_t inputSize)
{
    size_t size = inputSize;

    if (inputSize % 3)
    {
        size += 3 - (inputSize % 3);
    }

    size /= 3;
    size *= 4;

    return size;
}

size_t
Base64DecodedSize(const char *base64, size_t len)
{
    size_t ret;
    size_t i;

    if (!base64)
    {
        return 0;
    }

    ret = len / 4 * 3;

    for (i = len; i > 0; i--)
    {
        if (base64[i] == '=')
        {
            ret--;
        }
        else
        {
            break;
        }
    }

    return ret;
}

char *
Base64Encode(const char *input, size_t len)
{
    char *out;
    size_t outLen;
    size_t i, j, v;

    if (!input || !len)
    {
        return NULL;
    }

    outLen = Base64EncodedSize(len);
    out = Malloc(outLen + 1);
    if (!out)
    {
        return NULL;
    }
    out[outLen] = '\0';

    for (i = 0, j = 0; i < len; i += 3, j += 4)
    {
        v = input[i];
        v = i + 1 < len ? v << 8 | input[i + 1] : v << 8;
        v = i + 2 < len ? v << 8 | input[i + 2] : v << 8;

        out[j] = Base64EncodeMap[(v >> 18) & 0x3F];
        out[j + 1] = Base64EncodeMap[(v >> 12) & 0x3F];

        if (i + 1 < len)
        {
            out[j + 2] = Base64EncodeMap[(v >> 6) & 0x3F];
        }
        else
        {
            out[j + 2] = '=';
        }
        if (i + 2 < len)
        {
            out[j + 3] = Base64EncodeMap[v & 0x3F];
        }
        else
        {
            out[j + 3] = '=';
        }
    }

    return out;
}

static int
Base64IsValidChar(char c)
{
    return (c >= '0' && c <= '9') ||
    (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    (c == '+') ||
    (c == '/') ||
    (c == '=');
}

char *
Base64Decode(const char *input, size_t len)
{
    size_t i, j;
    int v;
    size_t outLen;
    char *out;

    if (!input)
    {
        return NULL;
    }

    outLen = Base64DecodedSize(input, len);
    if (len % 4)
    {
        /* Invalid length; must have incorrect padding */
        return NULL;
    }

    /* Scan for invalid characters. */
    for (i = 0; i < len; i++)
    {
        if (!Base64IsValidChar(input[i]))
        {
            return NULL;
        }
    }

    out = Malloc(outLen + 1);
    if (!out)
    {
        return NULL;
    }

    out[outLen] = '\0';

    for (i = 0, j = 0; i < len; i += 4, j += 3)
    {
        v = Base64DecodeMap[input[i] - 43];
        v = (v << 6) | Base64DecodeMap[input[i + 1] - 43];
        v = input[i + 2] == '=' ? v << 6 : (v << 6) | Base64DecodeMap[input[i + 2] - 43];
        v = input[i + 3] == '=' ? v << 6 : (v << 6) | Base64DecodeMap[input[i + 3] - 43];

        out[j] = (v >> 16) & 0xFF;
        if (input[i + 2] != '=')
            out[j + 1] = (v >> 8) & 0xFF;
        if (input[i + 3] != '=')
            out[j + 2] = v & 0xFF;
    }

    return out;
}

extern void
Base64Unpad(char *base64, size_t length)
{
    if (!base64)
    {
        return;
    }

    while (base64[length - 1] == '=')
    {
        length--;
    }

    base64[length] = '\0';
}

extern int
Base64Pad(char **base64Ptr, size_t length)
{
    char *tmp;
    size_t newSize;
    size_t i;

    if (length % 4 == 0)
    {
        return length;             /* Success: no padding needed */
    }

    newSize = length + (4 - (length % 4));

    tmp = Realloc(*base64Ptr, newSize + 100);;
    if (!tmp)
    {
        return 0;                  /* Memory error */
    }
    *base64Ptr = tmp;

    for (i = length; i < newSize; i++)
    {
        (*base64Ptr)[i] = '=';
    }

    (*base64Ptr)[newSize] = '\0';

    return newSize;
}
