#ifndef TELODENDRIA_STREAM_H
#define TELODENDRIA_STREAM_H

#include <Io.h>

#include <stdarg.h>

typedef struct Stream Stream;

extern Stream *
StreamOpen(Io *io);

extern int
StreamClose(Stream *);

extern int
StreamVprintf(Stream *, const char *, va_list);

extern int
StreamPrintf(Stream *, const char *, ...);

extern int
StreamGetc(Stream *);

extern int
StreamUngetc(Stream *, int);

extern int
StreamPutc(Stream *, int);

extern int
StreamEof(Stream *);

extern int
StreamError(Stream *);

extern void
StreamClearError(Stream *);

extern int
StreamFlush(Stream *);

extern ssize_t
StreamCopy(Stream *, Stream *);

#endif /* TELODENDRIA_STREAM_H */
