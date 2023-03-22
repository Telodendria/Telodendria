#ifndef TELODENDRIA_TLS_H
#define TELODENDRIA_TLS_H

#define TLS_LIBRESSL 1

/*
 * Other TLS_* macros can be declared here as support
 * for other implementations is added.
 */

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
