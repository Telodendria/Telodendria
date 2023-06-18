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
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <Telodendria.h>
#include <HttpServer.h>
#include <Util.h>

#include <Memory.h>
#include <Log.h>
#include <Db.h>
#include <Json.h>

static void
HexDump(size_t off, char *hexBuf, char *asciiBuf, void *args)
{
    char *fmt;

    (void) args;

    if (hexBuf && asciiBuf)
    {
        fmt = "%04lx: | %s | %s |";
    }
    else
    {
        fmt = "%04lx";
    }

    Log(LOG_DEBUG, fmt, off, hexBuf, asciiBuf);
}

void
TelodendriaMemoryHook(MemoryAction a, MemoryInfo * i, void *args)
{
    char *action;
    int err = 0;

    (void) args;

    switch (a)
    {
        case MEMORY_ALLOCATE:
            action = "Allocated";
            break;
        case MEMORY_REALLOCATE:
            action = "Re-allocated";
            break;
        case MEMORY_FREE:
            action = "Freed";
            break;
        case MEMORY_BAD_POINTER:
            err = 1;
            action = "Bad pointer to";
            break;
        case MEMORY_CORRUPTED:
            err = 1;
            action = "Corrupted block of";
            break;
        default:
            action = "Unknown operation on";
            break;
    }

    Log(err ? LOG_ERR : LOG_DEBUG,
        "%s:%d: %s %lu bytes of memory at %p.",
        MemoryInfoGetFile(i), MemoryInfoGetLine(i),
        action, MemoryInfoGetSize(i),
        MemoryInfoGetPointer(i));

    if (a != MEMORY_ALLOCATE && a != MEMORY_REALLOCATE)
    {
        MemoryHexDump(i, HexDump, NULL);
    }

    if (err)
    {
        raise(SIGINT);
    }
}

static HttpServer *server = NULL;

static void
SignalHandle(int signal)
{
    (void) signal;
    HttpServerStop(server);
}
struct Args
{
    Db *db;
    HttpRouter *router;
    HttpServerContext *cx;
};

static void *
TestFunc(Array * path, void *argp)
{
    struct Args *args = argp;
    HttpServerContext *cx = args->cx;
    HashMap *headers = HttpRequestHeaders(cx);
    HttpRequestMethod method = HttpRequestMethodGet(cx);
    Db *db = args->db;

    char *key;
    char *val;

    size_t bytes;

    DbRef *ref;

    (void) args;
    (void) path;

    Log(LOG_INFO, "%s %s", HttpRequestMethodToString(method),
        HttpRequestPath(cx));

    while (HashMapIterate(headers, &key, (void **) &val))
    {
        Log(LOG_INFO, "%s: %s", key, val);
    }

    Log(LOG_INFO, "");

    bytes = StreamCopy(HttpServerStream(cx), StreamStdout());

    StreamPutc(StreamStdout(), '\n');

    Log(LOG_DEBUG, "(%lu bytes)", bytes);

    ref = DbLock(db, 1, "test");


    HttpSendHeaders(cx);

    JsonEncode(DbJson(ref), HttpServerStream(cx), JSON_DEFAULT);
    DbUnlock(db, ref);

    return NULL;
}

void
HttpHandle(HttpServerContext * cx, void *argp)
{
    struct Args *args = argp;

    args->cx = cx;

    HttpRouterRoute(args->router, HttpRequestPath(cx), args, NULL);
}

int
Main(void)
{
    struct sigaction sa;
    HttpServerConfig cfg;

    struct Args args;

    LogConfigLevelSet(LogConfigGlobal(), LOG_DEBUG);

    Log(LOG_INFO, "Setting memory hook...");

    MemoryHook(TelodendriaMemoryHook, NULL);

    memset(&cfg, 0, sizeof(HttpServerConfig));

    cfg.flags = HTTP_FLAG_NONE;
    cfg.port = 8008;
    cfg.threads = 1;
    cfg.maxConnections = 1;
    cfg.handler = HttpHandle;

    cfg.handlerArgs = &args;

    args.db = DbOpen("data", 0);
    args.router = HttpRouterCreate();

    HttpRouterAdd(args.router, "/test", TestFunc);

    Log(LOG_DEBUG, "Creating server...");

    server = HttpServerCreate(&cfg);

    Log(LOG_DEBUG, "Starting server...");

    if (!HttpServerStart(server))
    {
        Log(LOG_ERR, "Unable to start HTTP server.");
        HttpServerFree(server);
        return 1;
    }

    Log(LOG_INFO, "Listening on port 8008.");

    sa.sa_handler = SignalHandle;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        Log(LOG_ERR, "Unable to install signal handler.");
        HttpServerStop(server);
        HttpServerJoin(server);
        HttpServerFree(server);
        return 1;
    }

    HttpServerJoin(server);

    Log(LOG_INFO, "Shutting down.");
    HttpServerStop(server);

    return 0;
}
