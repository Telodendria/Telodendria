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
#ifndef TELODENDRIA_IO_H
#define TELODENDRIA_IO_H

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef IO_BUFFER
#define IO_BUFFER 4096
#endif

typedef struct Io Io;

typedef ssize_t(IoReadFunc) (void *, void *, size_t);
typedef ssize_t(IoWriteFunc) (void *, void *, size_t);
typedef off_t(IoSeekFunc) (void *, off_t, int);
typedef int (IoCloseFunc) (void *);

typedef struct IoFunctions
{
    IoReadFunc *read;
    IoWriteFunc *write;
    IoSeekFunc *seek;
    IoCloseFunc *close;
} IoFunctions;

extern Io *
 IoCreate(void *, IoFunctions);

extern ssize_t
 IoRead(Io *, void *, size_t);

extern ssize_t
 IoWrite(Io *, void *, size_t);

extern off_t
 IoSeek(Io *, off_t, int);

extern int
 IoClose(Io *);

extern int
 IoVprintf(Io *, const char *, va_list);

extern int
 IoPrintf(Io *, const char *,...);

extern ssize_t
 IoCopy(Io *, Io *);

extern Io *
 IoFd(int);

extern Io *
 IoOpen(const char *, int, mode_t);

extern Io *
 IoFile(FILE *);

#endif                             /* TELODENDRIA_IO_H */
