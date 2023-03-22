#ifndef TELODENDRIA_TLS_H
#define TELODENDRIA_TLS_H

#define TLS_LIBRESSL 1
#define TLS_MBEDTLS 2
#define TLS_OPENSSL 3

#include <Stream.h>

extern Stream *
TlsClientStream(int, const char *);

extern Stream *
TlsServerStream(int, const char *, const char *);

/*
 * These are provided by individual TLS implementations.
 */

extern void *
TlsInitClient(int, const char *);

extern void *
TlsInitServer(int, const char *, const char *);

extern ssize_t
TlsRead(void *, void *, size_t);

extern ssize_t
TlsWrite(void *, void *, size_t);

extern int
TlsClose(void *);

#endif /* TELODENDRIA_TLS_H */
