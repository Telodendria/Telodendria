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
#ifndef CYTOPLASM_STREAM_H
#define CYTOPLASM_STREAM_H

/***
 * @Nm Stream
 * @Nd An abstraction over the Io API that implements standard C I/O.
 * @Dd April 29 2023
 * @Xr Io
 *
 * .Nm
 * implements an abstraction layer over the Io API. This layer buffers
 * I/O and makes it much easier to work with, mimicking the standard
 * C library and offering some more convenience features.
 */

#include <Io.h>

#include <stdarg.h>

/**
 * An opaque structure analogous to C's FILE pointers.
 */
typedef struct Stream Stream;

/**
 * Create a new stream using the specified Io for underlying I/O
 * operations.
 */
extern Stream * StreamIo(Io * io);

/**
 * Create a new stream using the specified POSIX file descriptor.
 * This is a convenience function for calling
 * .Fn IoFd
 * and then passing the result into
 * .Fn StreamIo .
 */
extern Stream * StreamFd(int);

/**
 * Create a new stream using the specified C FILE pointer. This is a
 * convenience function for calling
 * .Fn IoFile
 * and then passing the result into
 * .Fn StreamIo .
 */
extern Stream * StreamFile(FILE *);

/**
 * Create a new stream using the specified path and mode. This is a
 * convenience function for calling
 * .Xr fopen 3
 * and then passing the result into
 * .Fn StreamFile .
 */
extern Stream * StreamOpen(const char *, const char *);

/**
 * Get a stream that writes to the standard output.
 */
extern Stream * StreamStdout(void);

/**
 * Get a stream that writes to the standard error.
 */
extern Stream * StreamStderr(void);

/**
 * Get a stream that reads from the standard input.
 */
extern Stream * StreamStdin(void);

/**
 * Close the stream. This flushes the buffers and closes the underlying
 * Io. It is analogous to the standard
 * .Xr fclose 3
 * function.
 */
extern int StreamClose(Stream *);

/**
 * Print a formatted string. This function is analogous to the standard
 * .Xr vfprintf 3
 * function.
 */
extern int StreamVprintf(Stream *, const char *, va_list);

/**
 * Print a formatted string. This function is analogous to the
 * standard
 * .Xr fprintf 3
 * function.
 */
extern int StreamPrintf(Stream *, const char *,...);

/**
 * Get a single character from a stream. This function is analogous to
 * the standard
 * .Xr fgetc 3
 * function.
 */
extern int StreamGetc(Stream *);

/**
 * Push a character back onto the input stream. This function is
 * analogous to the standard
 * .Xr ungetc 3
 * function.
 */
extern int StreamUngetc(Stream *, int);

/**
 * Write a single character to the stream. This function is analogous
 * to the standard
 * .Xr fputc 3
 * function.
 */
extern int StreamPutc(Stream *, int);

/**
 * Write a null-terminated string to the stream. This function is
 * analogous to the standard
 * .Xr fputs 3
 * function.
 */
extern int StreamPuts(Stream *, char *);

/**
 * Read at most the specified number of characters minus 1 from the
 * specified stream and store them at the memory located at the
 * specified pointer. This function is analogous to the standard
 * .Xr fgets 3
 * function.
 */
extern char * StreamGets(Stream *, char *, int);

/**
 * Set the file position indicator for the specified stream. This
 * function is analogous to the standard
 * .Xr fseeko
 * function.
 */
extern off_t StreamSeek(Stream *, off_t, int);

/**
 * Test the end-of-file indicator for the given stream, returning a
 * boolean value indicating whether or not it is set. This is analogous
 * to the standard
 * .Xr feof 3
 * function.
 */
extern int StreamEof(Stream *);

/**
 * Test the stream for an error condition, returning a boolean value
 * indicating whether or not one is present. This is analogous to the
 * standard
 * .Xr ferror 3
 * function.
 */
extern int StreamError(Stream *);

/**
 * Clear the error condition associated with the given stream, allowing
 * future reads or writes to potentially be successful. This functio
 * is analogous to the standard
 * .Xr clearerr 3
 * function.
 */
extern void StreamClearError(Stream *);

/**
 * Flush all buffered data using the streams underlying write function.
 * This function is analogous to the standard
 * .Xr fflush 3
 * function.
 */
extern int StreamFlush(Stream *);

/**
 * Read all the bytes from the first stream and write them to the
 * second stream. This is analogous to
 * .Fn IoCopy ,
 * but it uses the internal buffers of the streams. It is probably
 * less efficient than doing a
 * .Fn IoCopy
 * instead, but it is more convenient.
 */
extern ssize_t StreamCopy(Stream *, Stream *);

/**
 * Get the file descriptor associated with the given stream, or -1 if
 * the stream is not associated with any file descriptor. This function
 * is analogous to the standard
 * .Xr fileno 3
 * function.
 */
extern int StreamFileno(Stream *);

#endif                             /* CYTOPLASM_STREAM_H */
