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
#include <Io.h>

#include <Memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

struct Io
{
    IoFunctions io;
    void *cookie;
};

Io *
IoCreate(void *cookie, IoFunctions funcs)
{
    Io *io;

    /* Must have at least read or write */
    if (!funcs.read && !funcs.write)
    {
        return NULL;
    }

    io = Malloc(sizeof(Io));

    if (!io)
    {
        return NULL;
    }

    io->cookie = cookie;

    io->io.read = funcs.read;
    io->io.write = funcs.write;
    io->io.seek = funcs.seek;
    io->io.close = funcs.close;

    return io;
}

ssize_t
IoRead(Io * io, void *buf, size_t nBytes)
{
    if (!io || !io->io.read)
    {
        errno = EBADF;
        return -1;
    }

    return io->io.read(io->cookie, buf, nBytes);
}

ssize_t
IoWrite(Io * io, void *buf, size_t nBytes)
{
    if (!io || !io->io.write)
    {
        errno = EBADF;
        return -1;
    }

    return io->io.write(io->cookie, buf, nBytes);
}

off_t
IoSeek(Io * io, off_t offset, int whence)
{
    if (!io)
    {
        errno = EBADF;
        return -1;
    }

    if (!io->io.seek)
    {
        errno = EINVAL;
        return -1;
    }

    return io->io.seek(io->cookie, offset, whence);
}

int
IoClose(Io * io)
{
    int ret;

    if (!io)
    {
        errno = EBADF;
        return -1;
    }

    if (io->io.close)
    {
        ret = io->io.close(io->cookie);
    }
    else
    {
        ret = 0;
    }

    Free(io);

    return ret;
}

int
IoVprintf(Io * io, const char *fmt, va_list ap)
{
    char *buf = NULL;
    size_t bufSize = 0;
    FILE *fp;

    int ret;

    if (!io || !fmt)
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

    if (ret >= 0)
    {
        ret = IoWrite(io, buf, bufSize);
    }

    free(buf);                     /* Allocated by stdlib, not Memory
                                    * API */
    return ret;
}

int
IoPrintf(Io * io, const char *fmt,...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = IoVprintf(io, fmt, ap);
    va_end(ap);

    return ret;
}

ssize_t
IoCopy(Io * in, Io * out)
{
    ssize_t nBytes = 0;
    char buf[IO_BUFFER];
    ssize_t rRes;
    ssize_t wRes;

    if (!in || !out)
    {
        errno = EBADF;
        return -1;
    }

    while ((rRes = IoRead(in, &buf, IO_BUFFER)) != 0)
    {
        if (rRes == -1)
        {
            return -1;
        }

        wRes = IoWrite(out, &buf, rRes);

        if (wRes == -1)
        {
            return -1;
        }

        nBytes += wRes;
    }

    return nBytes;
}
