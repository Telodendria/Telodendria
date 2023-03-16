#ifndef TELODENDRIA_IO_H
#define TELODENDRIA_IO_H

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#ifndef IO_BUFFER
#define IO_BUFFER 4096
#endif

typedef struct Io Io;

typedef ssize_t (IoReadFunc) (void *, void *, size_t);
typedef ssize_t (IoWriteFunc) (void *, void *, size_t);
typedef off_t (IoSeekFunc) (void *, off_t, int);
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
IoPrintf(Io *, const char *, ...);

extern ssize_t
IoCopy(Io *, Io *);

extern Io *
IoFd(int);

extern Io *
IoOpen(const char *, int, mode_t);

extern Io *
IoFile(FILE *);

#endif /* TELODENDRIA_IO_H */
