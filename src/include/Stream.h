#ifndef TELODENDRIA_STREAM_H
#define TELODENDRIA_STREAM_H

#include <unistd.h>
#include <stdarg.h>

typedef struct Stream Stream;

/*
 * Low-level Stream API in the style of POSIX system calls.
 * Heavily inspired by GNU fopencookie() and BSD funopen().
 */

typedef ssize_t (StreamReadFunc) (void *, void *, size_t);
typedef ssize_t (StreamWriteFunc) (void *, void *, size_t);
typedef off_t (StreamSeekFunc) (void *, off_t, int);
typedef int (StreamCloseFunc) (void *);

typedef struct StreamFunctions
{
    StreamReadFunc *read;
    StreamWriteFunc *write;
    StreamSeekFunc *seek;
    StreamCloseFunc *close;
} StreamFunctions;

extern Stream *
StreamCreate(void *, StreamFunctions);

extern ssize_t
StreamRead(Stream *, void *, size_t);

extern ssize_t
StreamWrite(Stream *, void *, size_t);

extern off_t
StreamSeek(Stream *, off_t, int);

extern int
StreamClose(Stream *);

extern Stream *
StreamOpen(int);

/*
 * High level Stream API in the style of C standard I/O.
 */

extern int
StreamVprintf(Stream *, const char *, va_list);

extern int
StreamPrintf(Stream *, const char *, ...);

extern int
StreamGetc(Stream *);

extern int
StreamUngetc(Stream *, int);

extern int
StreamEof(Stream *);

extern int
StreamError(Stream *);

extern void
StreamClearError(Stream *);

extern int
StreamFlush(Stream *);

#endif /* TELODENDRIA_STREAM_H */
