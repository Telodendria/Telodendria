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
#include <Tls.h>

#if TLS_IMPL == TLS_LIBRESSL

#include <Memory.h>
#include <Log.h>

#include <errno.h>

#include <tls.h>                   /* LibreSSL TLS */

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

    if (tls_handshake(cookie->ctx) == -1)
    {
        goto error;
    }

    return cookie;

error:
    if (cookie->ctx)
    {
        if (tls_error(cookie->ctx))
        {
            Log(LOG_ERR, "TlsInitClient(): %s", tls_error(cookie->ctx));
        }

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

    if (tls_accept_fds(cookie->ctx, &cookie->cctx, fd, fd) == -1)
    {
        goto error;
    }

    if (tls_handshake(cookie->cctx) == -1)
    {
        goto error;
    }

    return cookie;

error:
    if (cookie->ctx)
    {
        if (tls_error(cookie->ctx))
        {
            Log(LOG_ERR, "TlsInitServer(): %s", tls_error(cookie->ctx));
        }
        tls_free(cookie->ctx);
    }

    if (cookie->cctx)
    {
        if (tls_error(cookie->cctx))
        {
            Log(LOG_ERR, "TlsInitServer(): %s", tls_error(cookie->cctx));
        }

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
    struct tls *ctx = tls->cctx ? tls->cctx : tls->ctx;
    ssize_t ret = tls_read(ctx, buf, nBytes);

    if (ret == -1)
    {
        errno = EIO;
    }
    else if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT)
    {
        errno = EAGAIN;
        ret = -1;
    }

    return ret;
}

ssize_t
TlsWrite(void *cookie, void *buf, size_t nBytes)
{
    LibreSSLCookie *tls = cookie;
    struct tls *ctx = tls->cctx ? tls->cctx : tls->ctx;
    ssize_t ret = tls_write(ctx, buf, nBytes);

    if (ret == -1)
    {
        errno = EIO;
    }
    else if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT)
    {
        errno = EAGAIN;
        ret = -1;
    }

    return ret;
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
