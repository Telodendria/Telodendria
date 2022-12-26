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
#include <Util.h>

#include <Memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

unsigned long
UtilServerTs(void)
{
    struct timeval tv;
    unsigned long ts;

    gettimeofday(&tv, NULL);
    ts = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    return ts;
}

unsigned long
UtilLastModified(char *path)
{
    struct stat st;
    unsigned long ts;

    if (stat(path, &st) == 0)
    {
        ts = (st.st_mtim.tv_sec * 1000) + (st.st_mtim.tv_nsec / 1000000);
        return ts;
    }
    else
    {
        return 0;
    }
}

int
UtilMkdir(const char *dir, const mode_t mode)
{
    char tmp[PATH_MAX];
    char *p = NULL;

    struct stat st;

    size_t len;

    len = strnlen(dir, PATH_MAX);
    if (!len || len == PATH_MAX)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(tmp, dir, len);
    tmp[len] = '\0';

    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = '\0';
    }

    if (stat(tmp, &st) == 0 && S_ISDIR(st.st_mode))
    {
        return 0;
    }

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;

            if (stat(tmp, &st) != 0)
            {
                if (mkdir(tmp, mode) < 0)
                {
                    /* errno already set by mkdir() */
                    return -1;
                }
            }
            else if (!S_ISDIR(st.st_mode))
            {
                errno = ENOTDIR;
                return -1;
            }

            *p = '/';
        }
    }

    if (stat(tmp, &st) != 0)
    {
        if (mkdir(tmp, mode) < 0)
        {
            /* errno already set by mkdir() */
            return -1;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

char *
UtilUtf8Encode(unsigned long utf8)
{
    char *str;

    str = Malloc(5 * sizeof(char));
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
    outStr = Malloc(len + 1);      /* For the null terminator */
    if (!outStr)
    {
        return NULL;
    }

    strcpy(outStr, inStr);

    return outStr;
}

char *
UtilStringConcat(char *str1, char *str2)
{
    char *ret;
    size_t str1Len, str2Len;

    str1Len = str1 ? strlen(str1) : 0;
    str2Len = str2 ? strlen(str2) : 0;

    ret = Malloc(str1Len + str2Len + 1);

    if (!ret)
    {
        return NULL;
    }

    if (str1 && str2)
    {
        strcpy(ret, str1);
        strcpy(ret + str1Len, str2);
    }
    else
    {
        if (str1)
        {
            strcpy(ret, str1);
        }
        else if (str2)
        {
            strcpy(ret, str2);
        }
        else
        {
            strcpy(ret, "");
        }
    }

    return ret;
}

int
UtilSleepMillis(long ms)
{
    struct timespec ts;
    int res;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    res = nanosleep(&ts, &ts);

    return res;
}

size_t
UtilParseBytes(char *str)
{
    size_t bytes = 0;

    while (*str)
    {
        if (isdigit((unsigned char) *str))
        {
            bytes *= 10;
            bytes += *str - '0';
        }
        else
        {
            switch (*str)
            {
                case 'K':
                    bytes *= 1024;
                    break;
                case 'M':
                    bytes *= pow(1024, 2);
                    break;
                case 'G':
                    bytes *= pow(1024, 3);
                    break;
                case 'k':
                    bytes *= 1000;
                    break;
                case 'm':
                    bytes *= pow(1000, 2);
                    break;
                case 'g':
                    bytes *= pow(1000, 3);
                    break;
                default:
                    return 0;
            }

            if (*(str + 1))
            {
                return 0;
            }
        }

        str++;
    }

    return bytes;
}

ssize_t
UtilGetDelim(char **linePtr, size_t * n, int delim, FILE * stream)
{
    char *curPos, *newLinePtr;
    size_t newLinePtrLen;
    int c;

    if (!linePtr || !n || !stream)
    {
        errno = EINVAL;
        return -1;
    }

    if (*linePtr == NULL)
    {
        *n = 128;

        if (!(*linePtr = Malloc(*n)))
        {
            errno = ENOMEM;
            return -1;
        }
    }

    curPos = *linePtr;

    while (1)
    {
        c = getc(stream);

        if (ferror(stream) || (c == EOF && curPos == *linePtr))
        {
            return -1;
        }

        if (c == EOF)
        {
            break;
        }

        if ((*linePtr + *n - curPos) < 2)
        {
            if (SSIZE_MAX / 2 < *n)
            {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE;
#endif
                return -1;
            }

            newLinePtrLen = *n * 2;

            if (!(newLinePtr = Realloc(*linePtr, newLinePtrLen)))
            {
                errno = ENOMEM;
                return -1;
            }

            curPos = newLinePtr + (curPos - *linePtr);
            *linePtr = newLinePtr;
            *n = newLinePtrLen;
        }

        *curPos++ = (char) c;

        if (c == delim)
        {
            break;
        }
    }

    *curPos = '\0';
    return (ssize_t) (curPos - *linePtr);
}

ssize_t
UtilGetLine(char **linePtr, size_t * n, FILE * stream)
{
    return UtilGetDelim(linePtr, n, '\n', stream);
}

char *
UtilRandomString(size_t len)
{
    static const char charset[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char *str;
    size_t i;

    unsigned int seed = UtilServerTs() * (unsigned long) pthread_self();

    if (!len)
    {
        return NULL;
    }

    str = Malloc(len + 1);
    if (!str)
    {
        return NULL;
    }

    for (i = 0; i < len; i++)
    {
        str[i] = charset[rand_r(&seed) % (sizeof(charset) - 1)];
    }

    str[len] = '\0';
    return str;
}
