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
