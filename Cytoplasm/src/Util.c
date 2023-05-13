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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX LONG_MAX
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

ssize_t
UtilGetDelim(char **linePtr, size_t * n, int delim, Stream * stream)
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
        c = StreamGetc(stream);

        if (StreamError(stream) || (c == EOF && curPos == *linePtr))
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
UtilGetLine(char **linePtr, size_t * n, Stream * stream)
{
    return UtilGetDelim(linePtr, n, '\n', stream);
}
