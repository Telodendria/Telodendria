#include <Stream.h>

#include <Memory.h>

#include <errno.h>

struct Stream
{
    StreamFunctions io;
    void *cookie;

    int *ub;
    size_t ubSize;
    size_t ubLen;
};

Stream *
StreamCreate(void *cookie, StreamFunctions funcs)
{
    Stream *stream;

    if (!funcs.read || !funcs.write)
    {
        return NULL;
    }

    stream = Malloc(sizeof(Stream));

    if (!stream)
    {
        return NULL;
    }

    stream->cookie = cookie;

    stream->io.read = funcs.read;
    stream->io.write = funcs.write;
    stream->io.seek = funcs.seek;
    stream->io.close = funcs.close;

    stream->ubSize = 0;

    return stream;
}

ssize_t
StreamRead(Stream *stream, void *buf, size_t nBytes)
{
    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    return stream->io.read(stream->cookie, buf, nBytes);
}

ssize_t
StreamWrite(Stream *stream, void *buf, size_t nBytes)
{
    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    return stream->io.write(stream->cookie, buf, nBytes);
}

off_t
StreamSeek(Stream *stream, off_t offset, int whence)
{
    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    if (!stream->io.seek)
    {
        errno = EINVAL;
        return -1;
    }

    return stream->io.seek(stream->cookie, offset, whence);
}

int
StreamClose(Stream *stream)
{
    int ret;

    if (!stream)
    {
        errno = EBADF;
        return -1;
    }

    if (stream->io.close)
    {
        ret = stream->io.close(stream->cookie);
    }
    else
    {
        ret = 0;
    }

    if (stream->ubSize)
    {
        Free(stream->ub);
    }

    Free(stream);

    return ret;
}

static ssize_t
StreamReadFd(void *cookie, void *buf, size_t nBytes)
{
    int fd = *((int *) cookie);

    return read(fd, buf, nBytes);
}

static ssize_t
StreamWriteFd(void *cookie, void *buf, size_t nBytes)
{
    int fd = *((int *) cookie);

    return write(fd, buf, nBytes);
}

static off_t
StreamSeekFd(void *cookie, off_t offset, int whence)
{
    int fd = *((int *) cookie);

    return lseek(fd, offset, whence);
}

static int
StreamCloseFd(void *cookie)
{
    int fd = *((int *) cookie);

    return close(fd);
}

Stream *
StreamOpen(int fd)
{
    int *fdp = Malloc(sizeof(int));
    StreamFunctions f;

    if (!fdp)
    {
        return NULL;
    }

    *fdp = fd;

    f.read = StreamReadFd;
    f.write = StreamWriteFd;
    f.seek = StreamSeekFd;
    f.close = StreamCloseFd;

    return StreamCreate(fdp, f);

}
