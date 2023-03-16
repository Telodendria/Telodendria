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
