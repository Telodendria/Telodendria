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

#if TLS_IMPL == TLS_OPENSSL

#include <Memory.h>
#include <Log.h>

#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct OpenSSLCookie
{
    int fd;
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL *ssl;
} OpenSSLCookie;

static char *
SSLErrorString(int err)
{
    switch (err)
    {
            case SSL_ERROR_NONE:
            return "No error.";
        case SSL_ERROR_ZERO_RETURN:
            return "The TLS/SSL connection has been closed.";
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
            return "The operation did not complete.";
        case SSL_ERROR_WANT_X509_LOOKUP:
            return "X509 lookup failed.";
        case SSL_ERROR_SYSCALL:
            return "I/O Error.";
        case SSL_ERROR_SSL:
            return "SSL library error.";
    }
    return NULL;
}

void *
TlsInitClient(int fd, const char *serverName)
{
    OpenSSLCookie *cookie;
    char errorStr[256];

    /*
     * TODO: Seems odd that this isn't needed to make the
     * connection... we should figure out how to verify the
     * certificate matches the server we think we're
     * connecting to.
     */
    (void) serverName;

    cookie = Malloc(sizeof(OpenSSLCookie));
    if (!cookie)
    {
        return NULL;
    }

    memset(cookie, 0, sizeof(OpenSSLCookie));

    cookie->method = TLS_client_method();
    cookie->ctx = SSL_CTX_new(cookie->method);
    if (!cookie->ctx)
    {
        goto error;
    }

    cookie->ssl = SSL_new(cookie->ctx);
    if (!cookie->ssl)
    {
        goto error;
    }

    if (!SSL_set_fd(cookie->ssl, fd))
    {
        goto error;
    }

    if (SSL_connect(cookie->ssl) <= 0)
    {
        goto error;
    }

    return cookie;

error:
    Log(LOG_ERR, "TlsClientInit(): %s", ERR_error_string(ERR_get_error(), errorStr));

    if (cookie->ssl)
    {
        SSL_shutdown(cookie->ssl);
        SSL_free(cookie->ssl);
    }

    close(cookie->fd);

    if (cookie->ctx)
    {
        SSL_CTX_free(cookie->ctx);
    }

    return NULL;
}

void *
TlsInitServer(int fd, const char *crt, const char *key)
{
    OpenSSLCookie *cookie;
    char errorStr[256];
    int acceptRet = 0;

    cookie = Malloc(sizeof(OpenSSLCookie));
    if (!cookie)
    {
        return NULL;
    }

    memset(cookie, 0, sizeof(OpenSSLCookie));

    cookie->method = TLS_server_method();
    cookie->ctx = SSL_CTX_new(cookie->method);
    if (!cookie->ctx)
    {
        Log(LOG_ERR, "TlsInitServer(): Unable to create SSL Context.");
        goto error;
    }

    if (SSL_CTX_use_certificate_file(cookie->ctx, crt, SSL_FILETYPE_PEM) <= 0)
    {
        Log(LOG_ERR, "TlsInitServer(): Unable to set certificate file: %s", crt);
        goto error;
    }

    if (SSL_CTX_use_PrivateKey_file(cookie->ctx, key, SSL_FILETYPE_PEM) <= 0)
    {
        Log(LOG_ERR, "TlsInitServer(): Unable to set key file: %s", key);
        goto error;
    }

    cookie->ssl = SSL_new(cookie->ctx);
    if (!cookie->ssl)
    {
        Log(LOG_ERR, "TlsInitServer(): Unable to create SSL object.");
        goto error;
    }

    if (!SSL_set_fd(cookie->ssl, fd))
    {
        Log(LOG_ERR, "TlsInitServer(): Unable to set file descriptor.");
        goto error;
    }

    while ((acceptRet = SSL_accept(cookie->ssl)) <= 0)
    {
        switch (SSL_get_error(cookie->ssl, acceptRet))
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
                continue;
            default:
                Log(LOG_ERR, "TlsInitServer(): Unable to accept connection.");
                goto error;
        }
    }

    return cookie;

error:
    if (SSL_get_error(cookie->ssl, acceptRet) == SSL_ERROR_SYSCALL)
    {
        Log(LOG_ERR, "TlsServerInit(): System error: %s", strerror(errno));
    }
    Log(LOG_ERR, "TlsServerInit(): %s", SSLErrorString(SSL_get_error(cookie->ssl, acceptRet)));
    Log(LOG_ERR, "TlsServerInit(): %s", ERR_error_string(ERR_get_error(), errorStr));

    if (cookie->ssl)
    {
        SSL_shutdown(cookie->ssl);
        SSL_free(cookie->ssl);
    }

    close(cookie->fd);

    if (cookie->ctx)
    {
        SSL_CTX_free(cookie->ctx);
    }

    Free(cookie);

    return NULL;
}

ssize_t
TlsRead(void *cookie, void *buf, size_t nBytes)
{
    OpenSSLCookie *ssl = cookie;
    int ret;

    ret = SSL_read(ssl->ssl, buf, nBytes);

    if (ret <= 0)
    {
        switch (SSL_get_error(ssl->ssl, ret))
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
                errno = EAGAIN;
                break;
            case SSL_ERROR_ZERO_RETURN:
                ret = 0;
                break;
            default:
                errno = EIO;
                break;
        }
        ret = -1;
    }

    return ret;
}

ssize_t
TlsWrite(void *cookie, void *buf, size_t nBytes)
{
    OpenSSLCookie *ssl = cookie;
    int ret;

    ret = SSL_write(ssl->ssl, buf, nBytes);

    if (ret <= 0)
    {
        switch (SSL_get_error(ssl->ssl, ret))
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
                errno = EAGAIN;
                break;
            case SSL_ERROR_ZERO_RETURN:
                ret = 0;
                break;
            default:
                errno = EIO;
                break;
        }
        ret = -1;
    }

    return ret;
}

int
TlsClose(void *cookie)
{
    OpenSSLCookie *ssl = cookie;

    while (SSL_shutdown(ssl->ssl) == 0);
    SSL_free(ssl->ssl);
    SSL_CTX_free(ssl->ctx);

#if 0
    close(ssl->fd);
#endif

    Free(ssl);

    return 0;
}

#endif
