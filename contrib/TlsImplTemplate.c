/*
 * Telodendria TLS Implementation Template File.
 *
 * This file can serve as a baseline for new TLS implementations.
 * Please consult the Tls(3) man page for details.
 */
#include <Tls.h>

#if TLS_IMPL == TLS_TEMPLATE /* Set your TLS_* implementation flag here */

/*
 * #include statements and any implementation structures
 * needed should go here.
 */

void *
TlsInitClient(int fd, const char *serverName)
{
    return NULL;
}

void *
TlsInitServer(int fd, const char *crt, const char *key)
{
    return NULL;
}

ssize_t
TlsRead(void *cookie, void *buf, size_t nBytes)
{
    return -1;
}

ssize_t
TlsWrite(void *cookie, void *buf, size_t nBytes)
{
    return -1;
}

int
TlsClose(void *cookie)
{
    return -1;
}

#endif
