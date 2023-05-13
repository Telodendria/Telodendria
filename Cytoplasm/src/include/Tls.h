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
#ifndef CYTOPLASM_TLS_H
#define CYTOPLASM_TLS_H

/***
 * @Nm Tls
 * @Nd Interface to platform-dependent TLS libraries.
 * @Dd April 29 2023
 * @Xr Stream Io
 *
 * .Nm
 * provides an interface to platform-dependent TLS libraries. It allows
 * Cytoplasm to support any TLS library with no changes to existing
 * code. Support for additional TLS libraries is added by creating a
 * new compilation unit that implements all the functions here, with
 * the exception of a few, which are noted.
 * .Pp
 * Currently, Cytoplasm has support for the following TLS libraries:
 * .Bl -bullet -offset indent
 * .It
 * LibreSSL
 * .It
 * OpenSSL
 * .El
 */

#include <Stream.h>

#define TLS_LIBRESSL 2
#define TLS_OPENSSL 3

/**
 * Create a new TLS client stream using the given file descriptor and
 * the given server hostname. The hostname should be used to verify
 * that the server actually is who it says it is.
 * .Pp
 * This function does not need to be implemented by the individual
 * TLS support stubs.
 */
extern Stream * TlsClientStream(int, const char *);

/**
 * Create a new TLS server stream using the given certificate and key
 * file, in the format natively supported by the TLS library.
 * .Pp
 * This function does not need to be implemented by the individual
 * TLS support stubs.
 */
extern Stream * TlsServerStream(int, const char *, const char *);

/**
 * Initialize a cookie that stores information about the given client
 * connection. This cookie will be passed into the other functions
 * defined by this API.
 */
extern void * TlsInitClient(int, const char *);

/**
 * Initialize a cookie that stores information about the given
 * server connection. This cookie will be passed into the other
 * functions defined by this API.
 */
extern void * TlsInitServer(int, const char *, const char *);

/**
 * Read from a TLS stream, decrypting it and storing the result in the
 * specified buffer. This function takes the cookie, buffer, and
 * number of decrypted bytes to read into it. See the documentation for
 * .Fn IoRead .
 */
extern ssize_t TlsRead(void *, void *, size_t);

/**
 * Write to a TLS stream, encrypting the buffer. This function takes
 * the cookie, buffer, and number of unencrypted bytes to write to
 * the stream. See the documentation for
 * .Fn IoWrite .
 */
extern ssize_t TlsWrite(void *, void *, size_t);

/**
 * Close the TLS stream, also freeing all memory associated with the
 * cookie.
 */
extern int TlsClose(void *);

#endif                             /* CYTOPLASM_TLS_H */
