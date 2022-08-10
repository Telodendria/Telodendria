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
#include <Util.h>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

long
UtilServerTs(void)
{
    struct timeval tv;
    long ts;

    gettimeofday(&tv, NULL);
    ts = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    return ts;
}

char *
UtilUtf8Encode(unsigned long utf8)
{
    char *str;

    str = malloc(5 * sizeof(char));
    if (!str)
    {
        return NULL;
    }

    if (utf8 <= 0x7F)              /* Plain ASCII */
    {
        str[0] = (char) utf8;
        str[1] = '\0';
    }
    else if (utf8 <= 0x07FF)       /* 2-byte */
    {
        str[0] = (char) (((utf8 >> 6) & 0x1F) | 0xC0);
        str[1] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[2] = '\0';
    }
    else if (utf8 <= 0xFFFF)       /* 3-byte */
    {
        str[0] = (char) (((utf8 >> 12) & 0x0F) | 0xE0);
        str[1] = (char) (((utf8 >> 6) & 0x3F) | 0x80);
        str[2] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[3] = '\0';
    }
    else if (utf8 <= 0x10FFFF)     /* 4-byte */
    {
        str[0] = (char) (((utf8 >> 18) & 0x07) | 0xF0);
        str[1] = (char) (((utf8 >> 12) & 0x3F) | 0x80);
        str[2] = (char) (((utf8 >> 6) & 0x3F) | 0x80);
        str[3] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[4] = '\0';
    }
    else
    {
        /* Send replacement character */
        str[0] = (char) 0xEF;
        str[1] = (char) 0xBF;
        str[2] = (char) 0xBD;
        str[3] = '\0';
    }

    return str;
}

char *
UtilStringDuplicate(char *inStr)
{
    size_t len;
    char *outStr;

    len = strlen(inStr);
    outStr = malloc(len + 1);      /* For the null terminator */
    if (!outStr)
    {
        return NULL;
    }

    strcpy(outStr, inStr);

    return outStr;
}
