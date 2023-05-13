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

#ifdef TLS_IMPL

#include <Io.h>
#include <Stream.h>

Stream *
TlsClientStream(int fd, const char *serverName)
{
    Io *io;
    void *cookie;
    IoFunctions funcs;

    cookie = TlsInitClient(fd, serverName);
    if (!cookie)
    {
        return NULL;
    }

    funcs.read = TlsRead;
    funcs.write = TlsWrite;
    funcs.seek = NULL;
    funcs.close = TlsClose;

    io = IoCreate(cookie, funcs);
    if (!io)
    {
        return NULL;
    }

    return StreamIo(io);
}

Stream *
TlsServerStream(int fd, const char *crt, const char *key)
{
    Io *io;
    void *cookie;
    IoFunctions funcs;

    cookie = TlsInitServer(fd, crt, key);
    if (!cookie)
    {
        return NULL;
    }

    funcs.read = TlsRead;
    funcs.write = TlsWrite;
    funcs.seek = NULL;
    funcs.close = TlsClose;

    io = IoCreate(cookie, funcs);
    if (!io)
    {
        return NULL;
    }

    return StreamIo(io);
}

#endif
