#include <Io.h>

#include <Memory.h>

#include <errno.h>
#include <stdio.h>
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
IoRead(Io *io, void *buf, size_t nBytes)
{
    if (!io || !io->io.read)
    {
        errno = EBADF;
        return -1;
    }

    return io->io.read(io->cookie, buf, nBytes);
}

ssize_t
IoWrite(Io *io, void *buf, size_t nBytes)
{
    if (!io || !io->io.write)
    {
        errno = EBADF;
        return -1;
    }

    return io->io.write(io->cookie, buf, nBytes);
}

off_t
IoSeek(Io *io, off_t offset, int whence)
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
IoClose(Io *io)
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
IoVprintf(Io *io, const char *fmt, va_list ap)
{
    char *buf;
    size_t write;

    int ret;

    if (!io || !fmt)
    {
        return -1;
    }

    buf = Malloc(IO_BUFFER);
    if (!buf)
    {
        return -1;
    }

    write = vsnprintf(buf, IO_BUFFER, fmt, ap);

    if (write < 0)
    {
        Free(buf);
        return write;
    }

    /* Number of bytes to write exceeded buffer size; this should
     * be rare, but may occasionally happen. If it does, realloc to
     * the correct size and try again.
     */
    if (write >= IO_BUFFER)
    {
        char *new = Realloc(buf, write + 1);
        if (!new)
        {
            Free(buf);
            return -1;
        }

        buf = new;

        /* This time we don't care about the return value */
        vsnprintf(buf, write, fmt, ap);
    }

    ret = IoWrite(io, buf, write);

    Free(buf);
    return ret;
}

int
IoPrintf(Io *io, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = IoVprintf(io, fmt, ap);
    va_end(ap);

    return ret;
}

ssize_t
IoCopy(Io *in, Io *out)
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
