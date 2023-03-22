#include <Tls.h>

#if TLS_IMPL == TLS_LIBRESSL

#include <Memory.h>
#include <tls.h> /* LibreSSL TLS */

typedef struct LibreSSLCookie
{
    int fd;
    struct tls *ctx;
    struct tls *cctx;
    struct tls_config *cfg;
} LibreSSLCookie;

void *
TlsInitClient(int fd, const char *serverName)
{
    LibreSSLCookie *cookie = Malloc(sizeof(LibreSSLCookie));

    if (!cookie)
    {
        return NULL;
    }

    cookie->ctx = tls_client();
    cookie->cctx = NULL;
    cookie->cfg = tls_config_new();
    cookie->fd = fd;


    if (!cookie->ctx || !cookie->cfg)
    {
        goto error;
    }

    if (tls_config_set_ca_file(cookie->cfg, tls_default_ca_cert_file()) == -1)
    {
        goto error;
    }

    if (tls_configure(cookie->ctx, cookie->cfg) == -1)
    {
        goto error;
    }

    if (tls_connect_socket(cookie->ctx, fd, serverName) == -1)
    {
        goto error;
    }

    return cookie;

error:
    if (cookie->ctx)
    {
        tls_free(cookie->ctx);
    }

    if (cookie->cfg)
    {
        tls_config_free(cookie->cfg);
    }

    Free(cookie);

    return NULL;
}

void *
TlsInitServer(int fd, const char *crt, const char *key)
{
    LibreSSLCookie *cookie = Malloc(sizeof(LibreSSLCookie));

    if (!cookie)
    {
        return NULL;
    }

    cookie->ctx = tls_server();
    cookie->cctx = NULL;
    cookie->cfg = tls_config_new();
    cookie->fd = fd;

    if (!cookie->ctx || !cookie->cfg)
    {
        goto error;
    }

    if (tls_config_set_cert_file(cookie->cfg, crt) == -1)
    {
        goto error;
    }

    if (tls_config_set_key_file(cookie->cfg, key) == -1)
    {
        goto error;
    }

    if (tls_configure(cookie->ctx, cookie->cfg) == -1)
    {
        goto error;
    }

    if (tls_accept_socket(cookie->ctx, &cookie->cctx, fd) == -1)
    {
        goto error;
    }

    return cookie;

error:
    if (cookie->ctx)
    {
        tls_free(cookie->ctx);
    }

    if (cookie->cctx)
    {
        tls_free(cookie->cctx);
    }

    if (cookie->cfg)
    {
        tls_config_free(cookie->cfg);
    }

    Free(cookie);

    return NULL;
}

ssize_t
TlsRead(void *cookie, void *buf, size_t nBytes)
{
    LibreSSLCookie *tls = cookie;

    return tls_read(tls->cctx ? tls->cctx : tls->ctx, buf, nBytes);
}

ssize_t
TlsWrite(void *cookie, void *buf, size_t nBytes)
{
    LibreSSLCookie *tls = cookie;

    return tls_write(tls->cctx ? tls->cctx : tls->ctx, buf, nBytes);
}

int
TlsClose(void *cookie)
{
    LibreSSLCookie *tls = cookie;

    int tlsRet = tls_close(tls->cctx ? tls->cctx : tls->ctx);
    int sdRet;

    if (tls->cctx)
    {
        tls_free(tls->cctx);
    }

    tls_free(tls->ctx);
    tls_config_free(tls->cfg);

    sdRet = close(tls->fd);

    Free(tls);

    return (tlsRet == -1 || sdRet == -1) ? -1 : 0;
}

#endif
