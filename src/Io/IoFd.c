#include <Io.h>

#include <Memory.h>

#include <fcntl.h>

static ssize_t
IoReadFd(void *cookie, void *buf, size_t nBytes)
{
    int fd = *((int *) cookie);

    return read(fd, buf, nBytes);
}

static ssize_t
IoWriteFd(void *cookie, void *buf, size_t nBytes)
{
    int fd = *((int *) cookie);

    return write(fd, buf, nBytes);
}

static off_t
IoSeekFd(void *cookie, off_t offset, int whence)
{
    int fd = *((int *) cookie);

    return lseek(fd, offset, whence);
}

static int
IoCloseFd(void *cookie)
{
    int fd = *((int *) cookie);

    Free(cookie);
    return close(fd);
}

Io *
IoFd(int fd)
{
    int *cookie = Malloc(sizeof(int));
    IoFunctions f;

    if (!cookie)
    {
        return NULL;
    }

    *cookie = fd;

    f.read = IoReadFd;
    f.write = IoWriteFd;
    f.seek = IoSeekFd;
    f.close = IoCloseFd;

    return IoCreate(cookie, f);
}

Io *
IoOpen(const char *path, int flags, mode_t mode)
{
    int fd = open(path, flags, mode);

    if (fd == -1)
    {
        return NULL;
    }

    return IoFd(fd);
}
