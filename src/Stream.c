#include <Stream.h>

#include <Io.h>
#include <Memory.h>
#include <Util.h>

#include <stdio.h>
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

struct Stream
{
    Io *io;

    int *rBuf;
    size_t rLen;
    size_t rOff;

    int *wBuf;
    size_t wLen;

    int *ugBuf;
    size_t ugSize;
    size_t ugLen;

    int flags:2;
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

    return stream;
}

Stream *
StreamFd(int fd)
{
    Io *io = IoFd(fd);

    if (!io)
    {
        return NULL;
    }

    return StreamIo(io);
}

Stream *
StreamOpen(const char *path, const char *mode)
{
    FILE *fp = fopen(path, mode);
    Io *io;

    if (!fp)
    {
        return NULL;
    }

    io = IoFile(fp);

    if (!io)
    {
        return NULL;
    }

    return StreamIo(io);
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
    if (!stream)
    {
        return -1;
    }

    StreamFlush(stream);           /* Flush the buffer out before doing
                                    * the printf */

    /* Defer printf to underlying Io. We probably should buffer the
     * printf operation just like StreamPutc() so we don't have to
     * flush the buffer. */
    return IoVprintf(stream->io, fmt, ap);
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

    if (stream->flags & EOF)
    {
        return EOF;
    }

    if (!stream->rBuf)
    {
        /* No buffer allocated yet */
        stream->rBuf = Malloc(IO_BUFFER * sizeof(int));
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
        int *new;

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

    stream->ugBuf[stream->ugLen - 1] = c;
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
        stream->wBuf = Malloc(IO_BUFFER * sizeof(int));
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

    return c;
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
