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
#include <Stream.h>

#include <Io.h>
#include <Memory.h>
#include <Util.h>
#include <Int.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifndef STREAM_RETRIES
#define STREAM_RETRIES 10
#endif

#ifndef STREAM_DELAY
#define STREAM_DELAY 2
#endif

#define STREAM_EOF (1 << 0)
#define STREAM_ERR (1 << 1)
#define STREAM_TTY (1 << 2)

struct Stream
{
    Io *io;

    UInt8 *rBuf;
    size_t rLen;
    size_t rOff;

    UInt8 *wBuf;
    size_t wLen;

    char *ugBuf;
    size_t ugSize;
    size_t ugLen;

    int flags;

    int fd;
};

Stream *
StreamIo(Io * io)
{
    Stream *stream;

    if (!io)
    {
        return NULL;
    }

    stream = Malloc(sizeof(Stream));
    if (!stream)
    {
        return NULL;
    }

    memset(stream, 0, sizeof(Stream));
    stream->io = io;
    stream->fd = -1;

    return stream;
}

Stream *
StreamFd(int fd)
{
    Io *io = IoFd(fd);
    Stream *stream;

    if (!io)
    {
        return NULL;
    }

    stream = StreamIo(io);
    if (!stream)
    {
        return NULL;
    }

    stream->fd = fd;

    if (isatty(stream->fd))
    {
        stream->flags |= STREAM_TTY;
    }

    return stream;
}

Stream *
StreamFile(FILE * fp)
{
    Io *io = IoFile(fp);
    Stream *stream;

    if (!io)
    {
        return NULL;
    }

    stream = StreamIo(io);
    if (!stream)
    {
        return NULL;
    }

    stream->fd = fileno(fp);

    if (isatty(stream->fd))
    {
        stream->flags |= STREAM_TTY;
    }

    return stream;
}

Stream *
StreamOpen(const char *path, const char *mode)
{
    FILE *fp = fopen(path, mode);

    if (!fp)
    {
        return NULL;
    }

    return StreamFile(fp);
}

Stream *
StreamStdout(void)
{
    static Stream *stdOut = NULL;

    if (!stdOut)
    {
        stdOut = StreamFd(STDOUT_FILENO);
    }

    return stdOut;
}

Stream *
StreamStderr(void)
{
    static Stream *stdErr = NULL;

    if (!stdErr)
    {
        stdErr = StreamFd(STDERR_FILENO);
    }

    return stdErr;
}

Stream *
StreamStdin(void)
{
    static Stream *stdIn = NULL;

    if (!stdIn)
    {
        stdIn = StreamFd(STDIN_FILENO);
    }

    return stdIn;
}

int
StreamClose(Stream * stream)
{
    int ret = 0;

    if (!stream)
    {
        errno = EBADF;
        return EOF;
    }

    if (stream->rBuf)
    {
        Free(stream->rBuf);
    }

    if (stream->wBuf)
    {
        ssize_t writeRes = IoWrite(stream->io, stream->wBuf, stream->wLen);

        Free(stream->wBuf);

        if (writeRes == -1)
        {
            ret = EOF;
        }
    }

    if (stream->ugBuf)
    {
        Free(stream->ugBuf);
    }

    ret = IoClose(stream->io);
    Free(stream);

    return ret;
}

int
StreamVprintf(Stream * stream, const char *fmt, va_list ap)
{
    /* This might look like very similar code to IoVprintf(), but I
     * chose not to defer to IoVprintf() because that would require us
     * to immediately flush the buffer, since the Io API is unbuffered.
     * StreamPuts() uses StreamPutc() under the hood, which is
     * buffered. It therefore allows us to finish filling the buffer
     * and then only flush it when necessary, preventing superfluous
     * writes. */

    char *buf = NULL;
    size_t bufSize = 0;
    FILE *fp;

    int ret;

    if (!fmt)
    {
        return -1;
    }

    fp = open_memstream(&buf, &bufSize);
    if (!fp)
    {
        return -1;
    }

    ret = vfprintf(fp, fmt, ap);
    fclose(fp);

    if (ret >= 0 && stream)
    {
        if (StreamPuts(stream, buf) < 0)
        {
            ret = -1;
        };
    }

    free(buf);                     /* Allocated by stdlib, not Memory
                                    * API */

    return ret;
}

int
StreamPrintf(Stream * stream, const char *fmt,...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = StreamVprintf(stream, fmt, ap);
    va_end(ap);

    return ret;
}

int
StreamGetc(Stream * stream)
{
    int c;

    if (!stream)
    {
        errno = EBADF;
        return EOF;
    }

    /* Empty the ungetc stack first */
    if (stream->ugLen)
    {
        c = stream->ugBuf[stream->ugLen - 1];
        stream->ugLen--;
        return c;
    }

    if (stream->flags & STREAM_EOF)
    {
        return EOF;
    }

    if (!stream->rBuf)
    {
        /* No buffer allocated yet */
        stream->rBuf = Malloc(IO_BUFFER);
        if (!stream->rBuf)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }

        stream->rOff = 0;
        stream->rLen = 0;
    }

    if (stream->rOff >= stream->rLen)
    {
        /* We read through the entire buffer; get a new one */
        ssize_t readRes = IoRead(stream->io, stream->rBuf, IO_BUFFER);

        if (readRes == 0)
        {
            stream->flags |= STREAM_EOF;
            return EOF;
        }

        if (readRes == -1)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }

        stream->rOff = 0;
        stream->rLen = readRes;
    }

    /* Read the character in the buffer and advance the offset */
    c = stream->rBuf[stream->rOff];
    stream->rOff++;

    return c;
}

int
StreamUngetc(Stream * stream, int c)
{
    if (!stream)
    {
        errno = EBADF;
        return EOF;
    }

    if (!stream->ugBuf)
    {
        stream->ugSize = IO_BUFFER;
        stream->ugBuf = Malloc(stream->ugSize);

        if (!stream->ugBuf)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }
    }

    if (stream->ugLen >= stream->ugSize)
    {
        char *new;

        stream->ugSize += IO_BUFFER;
        new = Realloc(stream->ugBuf, stream->ugSize);
        if (!new)
        {
            stream->flags |= STREAM_ERR;
            Free(stream->ugBuf);
            stream->ugBuf = NULL;
            return EOF;
        }

        Free(stream->ugBuf);
        stream->ugBuf = new;
    }

    stream->ugBuf[stream->ugLen] = c;
    stream->ugLen++;

    return c;
}

int
StreamPutc(Stream * stream, int c)
{
    if (!stream)
    {
        errno = EBADF;
        return EOF;
    }

    if (!stream->wBuf)
    {
        stream->wBuf = Malloc(IO_BUFFER);
        if (!stream->wBuf)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }
    }

    if (stream->wLen == IO_BUFFER)
    {
        /* Buffer full; write it */
        ssize_t writeRes = IoWrite(stream->io, stream->wBuf, stream->wLen);

        if (writeRes == -1)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }

        stream->wLen = 0;
    }

    stream->wBuf[stream->wLen] = c;
    stream->wLen++;

    if (stream->flags & STREAM_TTY && c == '\n')
    {
        /* Newline encountered on a TTY; write now. This fixes some
         * strange behavior on certain TTYs where a newline is written
         * to the screen upon flush even when no newline exists in the
         * stream. We just flush on newlines, but only if we're
         * directly writing to a TTY. */
        ssize_t writeRes = IoWrite(stream->io, stream->wBuf, stream->wLen);

        if (writeRes == -1)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }

        stream->wLen = 0;
    }

    return c;
}

int
StreamPuts(Stream * stream, char *str)
{
    int ret = 0;

    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    while (*str)
    {
        if (StreamPutc(stream, *str) == EOF)
        {
            ret = -1;
            break;
        }

        str++;
    }

    return ret;
}

char *
StreamGets(Stream * stream, char *str, int size)
{
    int i;

    if (!stream)
    {
        errno = EBADF;
        return NULL;
    }

    if (size <= 0)
    {
        errno = EINVAL;
        return NULL;
    }

    for (i = 0; i < size - 1; i++)
    {
        int c = StreamGetc(stream);

        if (StreamEof(stream) || StreamError(stream))
        {
            break;
        }

        str[i] = c;

        if (c == '\n')
        {
            i++;
            break;
        }
    }

    str[i] = '\0';

    return str;
}

off_t
StreamSeek(Stream * stream, off_t offset, int whence)
{
    off_t result;

    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    result = IoSeek(stream->io, offset, whence);
    if (result < 0)
    {
        return result;
    }

    /* Successful seek; clear the buffers */
    stream->rOff = 0;
    stream->wLen = 0;
    stream->ugLen = 0;

    return result;
}

int
StreamEof(Stream * stream)
{
    return stream && (stream->flags & STREAM_EOF);
}

int
StreamError(Stream * stream)
{
    return stream && (stream->flags & STREAM_ERR);
}

void
StreamClearError(Stream * stream)
{
    if (stream)
    {
        stream->flags &= ~STREAM_ERR;
    }
}

int
StreamFlush(Stream * stream)
{
    if (!stream)
    {
        errno = EBADF;
        return EOF;
    }

    if (stream->wLen)
    {
        ssize_t writeRes = IoWrite(stream->io, stream->wBuf, stream->wLen);

        if (writeRes == -1)
        {
            stream->flags |= STREAM_ERR;
            return EOF;
        }

        stream->wLen = 0;
    }

    return 0;
}

ssize_t
StreamCopy(Stream * in, Stream * out)
{
    ssize_t nBytes = 0;
    int c;
    int tries = 0;
    int readFlg = 0;

    while (1)
    {
        c = StreamGetc(in);

        if (StreamEof(in))
        {
            break;
        }

        if (StreamError(in))
        {
            if (errno == EAGAIN)
            {
                StreamClearError(in);
                tries++;

                if (tries >= STREAM_RETRIES || readFlg)
                {
                    break;
                }
                else
                {
                    UtilSleepMillis(STREAM_DELAY);
                    continue;
                }
            }
            else
            {
                break;
            }
        }

        /* As soon as we've successfully read a byte, treat future
         * EAGAINs as EOF, because somebody might have forgotten to
         * close their stream. */

        readFlg = 1;
        tries = 0;

        StreamPutc(out, c);
        nBytes++;
    }

    StreamFlush(out);
    return nBytes;
}

int
StreamFileno(Stream * stream)
{
    return stream ? stream->fd : -1;
}
