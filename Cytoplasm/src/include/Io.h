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
#ifndef CYTOPLASM_IO_H
#define CYTOPLASM_IO_H

/***
 * @Nm Io
 * @Nd Source/sink-agnostic I/O for implementing custom streams.
 * @Dd April 29 2023
 * @Xr Stream Tls
 *
 * Many systems provide platform-specific means of implementing custom
 * streams using file pointers. However, POSIX does not define a way
 * of programmatically creating custom streams.
 * .Nm
 * therefore fills this gap in POSIX by mimicking all of the
 * functionality of these platform-specific functions, but in pure
 * POSIX C. It defines a number of callback funtions to be executed
 * in place of the standard POSIX I/O functions, which are used to
 * implement arbitrary streams that may not be to a file or socket.
 * Additionally, streams can now be pipelined; the sink of one stream
 * may be the source of another lower-level stream. Additionally, all
 * streams, regardless of their source or sink, share the same API, so
 * streams can be handled in a much more generic manner. This allows
 * the HTTP client and server libraries to seemlessly support TLS and
 * plain connections without having to handle each separately.
 * .Pp
 * .Nm
 * was heavily inspired by GNU's
 * .Fn fopencookie
 * and BSD's
 * .Fn funopen .
 * It aims to combine the best of both of these functions into a single
 * API that is intuitive and easy to use.
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef IO_BUFFER
#define IO_BUFFER 4096
#endif

/**
 * An opaque structure analogous to a POSIX file descriptor.
 */
typedef struct Io Io;

/**
 * Read input from the source of a stream. This function should
 * attempt to read the specified number of bytes of data from the
 * given cookie into the given buffer. It should behave identically
 * to the POSIX
 * .Xr read 2
 * system call, except instead of using an integer descriptor as the
 * first parameter, a pointer to an implementation-defined cookie
 * stores any information the function needs to read from the source.
 */
typedef ssize_t (IoReadFunc) (void *, void *, size_t);

/**
 * Write output to a sink. This function should attempt to write the
 * specified number of bytes of data from the given buffer into the
 * stream described by the given cookie. It should behave identically
 * to the POSIX
 * .Xr write 2
 * system call, except instead of using an integer descriptor as the
 * first parameter, a pointer to an implementation-defined cookie
 * stores any information the function needs to write to the sink.
 */
typedef ssize_t (IoWriteFunc) (void *, void *, size_t);

/**
 * Repositions the offset of the stream described by the specified
 * cookie. This function should behave identically to the POSIX
 * .Xr lseek 2
 * system call, except instead of using an integer descriptor as the
 * first parameter, a pointer to an implementation-defined cookie
 * stores any information the function needs to seek the stream.
 */
typedef off_t (IoSeekFunc) (void *, off_t, int);

/**
 * Close the given stream, making future reads or writes result in
 * undefined behavior. This function should also free all memory
 * associated with the cookie. It should behave identically to the
 * .Xr close 2
 * system call, except instead of using an integer descriptor for the
 * parameter, a pointer to an implementation-defined cookie stores any
 * information the function needs to close the stream.
 */
typedef int (IoCloseFunc) (void *);

/**
 * A simple mechanism for grouping together a set of stream functions,
 * to be passed to
 * .Fn IoCreate .
 */
typedef struct IoFunctions
{
    IoReadFunc *read;
    IoWriteFunc *write;
    IoSeekFunc *seek;
    IoCloseFunc *close;
} IoFunctions;

/**
 * Create a new stream using the specified cookie and the specified
 * I/O functions.
 */
extern Io * IoCreate(void *, IoFunctions);

/**
 * Read the specified number of bytes from the specified stream into
 * the specified buffer. This calls the stream's underlying IoReadFunc,
 * which should behave identically to the POSIX
 * .Xr read 2
 * system call.
 */
extern ssize_t IoRead(Io *, void *, size_t);

/**
 * Write the specified number of bytes from the specified stream into
 * the specified buffer. This calls the stream's underlying
 * IoWriteFunc, which should behave identically to the POSIX
 * .Xr write 2
 * system call.
 */
extern ssize_t IoWrite(Io *, void *, size_t);

/**
 * Seek the specified stream using the specified offset and whence
 * value. This calls the stream's underlying IoSeekFunc, which should
 * behave identically to the POSIX
 * .Xr lseek 2
 * system call.
 */
extern off_t IoSeek(Io *, off_t, int);

/**
 * Close the specified stream. This calls the stream's underlying
 * IoCloseFunc, which should behave identically to the POSIX
 * .Xr close 2
 * system call.
 */
extern int IoClose(Io *);

/**
 * Print a formatted string to the given stream. This is a
 * re-implementation of the standard library function
 * .Xr vfprintf 3 ,
 * and behaves identically.
 */
extern int IoVprintf(Io *, const char *, va_list);

/**
 * Print a formatted string to the given stream. This is a
 * re-implementation of the standard library function
 * .Xr fprintf 3 ,
 * and behaves identically.
 */
extern int IoPrintf(Io *, const char *,...);

/**
 * Read all the bytes from the first stream and write them to the
 * second stream. Neither stream is closed upon the completion of this
 * function. This can be used for quick and convenient buffered
 * copying of data from one stream into another.
 */
extern ssize_t IoCopy(Io *, Io *);

/**
 * Wrap a POSIX file descriptor to take advantage of this API. The
 * common use case for this function is when a regular file descriptor
 * needs to be accessed by an application that uses this API to also
 * access non-POSIX streams.
 */
extern Io * IoFd(int);

/**
 * Open or create a file for reading or writing. The specified file
 * name is opened for reading or writing as specified by the given
 * flags and mode. This function is a simple convenience wrapper around
 * the POSIX
 * .Xr open 2
 * system call that passes the opened file descriptor into
 * .Fn IoFd .
 */
extern Io * IoOpen(const char *, int, mode_t);

/**
 * Wrap a standard C file pointer to take advantage of this API. The
 * common use case for this function is when a regular C file pointer
 * needs to be accessed by an application that uses this API to also
 * access custom streams.
 */
extern Io * IoFile(FILE *);

#endif                             /* CYTOPLASM_IO_H */
