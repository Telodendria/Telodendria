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
#include <HttpServer.h>
#include <Memory.h>
#include <Queue.h>
#include <Array.h>
#include <Util.h>
#include <Tls.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <errno.h>

static const char ENABLE = 1;

struct HttpServer
{
    int sd;
    unsigned int nThreads;
    unsigned int maxConnections;
    pthread_t socketThread;
    int flags;
    char *tlsCrt;
    char *tlsKey;

    volatile unsigned int stop:1;
    volatile unsigned int isRunning:1;

    HttpHandler *requestHandler;
    void *handlerArgs;

    Queue *connQueue;
    pthread_mutex_t connQueueMutex;

    Array *threadPool;
};

struct HttpServerContext
{
    HashMap *requestHeaders;
    HttpRequestMethod requestMethod;
    char *requestPath;
    HashMap *requestParams;

    HashMap *responseHeaders;
    HttpStatus responseStatus;

    Stream *stream;
};

typedef struct HttpServerWorkerThreadArgs
{
    HttpServer *server;
    int id;
    pthread_t thread;
} HttpServerWorkerThreadArgs;

static HttpServerContext *
HttpServerContextCreate(HttpRequestMethod requestMethod,
          char *requestPath, HashMap * requestParams, Stream * stream)
{
    HttpServerContext *c;

    c = Malloc(sizeof(HttpServerContext));
    if (!c)
    {
        return NULL;
    }

    c->responseHeaders = HashMapCreate();
    if (!c->responseHeaders)
    {
        Free(c->requestHeaders);
        Free(c);
        return NULL;
    }

    c->requestMethod = requestMethod;
    c->requestPath = requestPath;
    c->requestParams = requestParams;
    c->stream = stream;
    c->responseStatus = HTTP_OK;

    return c;
}

static void
HttpServerContextFree(HttpServerContext * c)
{
    char *key;
    void *val;

    if (!c)
    {
        return;
    }

    while (HashMapIterate(c->requestHeaders, &key, &val))
    {
        Free(val);
    }
    HashMapFree(c->requestHeaders);

    while (HashMapIterate(c->responseHeaders, &key, &val))
    {
        /*
         * These are generated by code. As such, they may be either
         * on the heap, or on the stack, depending on how they were
         * added.
         *
         * Basically, if the memory API knows about a pointer, then
         * it can be freed. If it doesn't know about a pointer, skip
         * freeing it because it's probably a stack pointer.
         */

        if (MemoryInfoGet(val))
        {
            Free(val);
        }
    }

    HashMapFree(c->responseHeaders);

    while (HashMapIterate(c->requestParams, &key, &val))
    {
        Free(val);
    }

    HashMapFree(c->requestParams);

    Free(c->requestPath);
    StreamClose(c->stream);

    Free(c);
}

HashMap *
HttpRequestHeaders(HttpServerContext * c)
{
    if (!c)
    {
        return NULL;
    }

    return c->requestHeaders;
}

HttpRequestMethod
HttpRequestMethodGet(HttpServerContext * c)
{
    if (!c)
    {
        return HTTP_METHOD_UNKNOWN;
    }

    return c->requestMethod;
}

char *
HttpRequestPath(HttpServerContext * c)
{
    if (!c)
    {
        return NULL;
    }

    return c->requestPath;
}

HashMap *
HttpRequestParams(HttpServerContext * c)
{
    if (!c)
    {
        return NULL;
    }

    return c->requestParams;
}

char *
HttpResponseHeader(HttpServerContext * c, char *key, char *val)
{
    if (!c)
    {
        return NULL;
    }

    return HashMapSet(c->responseHeaders, key, val);
}

void
HttpResponseStatus(HttpServerContext * c, HttpStatus status)
{
    if (!c)
    {
        return;
    }

    c->responseStatus = status;
}

Stream *
HttpServerStream(HttpServerContext * c)
{
    if (!c)
    {
        return NULL;
    }

    return c->stream;
}

void
HttpSendHeaders(HttpServerContext * c)
{
    Stream *fp = c->stream;

    char *key;
    char *val;

    StreamPrintf(fp, "HTTP/1.0 %d %s\n", c->responseStatus, HttpStatusToString(c->responseStatus));

    while (HashMapIterate(c->responseHeaders, &key, (void **) &val))
    {
        StreamPrintf(fp, "%s: %s\n", key, val);
    }

    StreamPuts(fp, "\n");
}

static Stream *
DequeueConnection(HttpServer * server)
{
    Stream *fp;

    if (!server)
    {
        return NULL;
    }

    pthread_mutex_lock(&server->connQueueMutex);
    fp = QueuePop(server->connQueue);
    pthread_mutex_unlock(&server->connQueueMutex);

    return fp;
}

HttpServer *
HttpServerCreate(int flags, unsigned short port, unsigned int nThreads, unsigned int maxConnections,
                 HttpHandler * requestHandler, void *handlerArgs)
{
    HttpServer *server;
    struct sockaddr_in sa;

    if (!requestHandler)
    {
        return NULL;
    }

#ifndef TLS_IMPL
    if (flags & HTTP_FLAG_TLS)
    {
        return NULL;
    }
#endif

    server = Malloc(sizeof(HttpServer));
    if (!server)
    {
        goto error;
    }

    memset(server, 0, sizeof(HttpServer));

    server->flags = flags;

    server->threadPool = ArrayCreate();
    if (!server->threadPool)
    {
        goto error;
    }

    server->connQueue = QueueCreate(maxConnections);
    if (!server->connQueue)
    {
        goto error;
    }

    if (pthread_mutex_init(&server->connQueueMutex, NULL) != 0)
    {
        goto error;
    }

    server->sd = socket(AF_INET, SOCK_STREAM, 0);

    if (server->sd < 0)
    {
        goto error;
    }

    if (fcntl(server->sd, F_SETFL, O_NONBLOCK) == -1)
    {
        goto error;
    }

    if (setsockopt(server->sd, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(int)) < 0)
    {
        goto error;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(server->sd, SOL_SOCKET, SO_REUSEPORT, &ENABLE, sizeof(int)) < 0)
    {
        goto error;
    }
#endif

    memset(&sa, 0, sizeof(struct sockaddr_in));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server->sd, (struct sockaddr *) & sa, sizeof(sa)) < 0)
    {
        goto error;
    }

    if (listen(server->sd, maxConnections) < 0)
    {
        goto error;
    }

    server->nThreads = nThreads;
    server->maxConnections = maxConnections;
    server->requestHandler = requestHandler;
    server->handlerArgs = handlerArgs;
    server->stop = 0;
    server->isRunning = 0;

    return server;

error:
    if (server)
    {
        if (server->connQueue)
        {
            QueueFree(server->connQueue);
        }

        pthread_mutex_destroy(&server->connQueueMutex);

        if (server->threadPool)
        {
            ArrayFree(server->threadPool);
        }

        if (server->sd)
        {
            close(server->sd);
        }

        Free(server);
    }
    return NULL;
}

void
HttpServerFree(HttpServer * server)
{
    if (!server)
    {
        return;
    }

    close(server->sd);
    QueueFree(server->connQueue);
    pthread_mutex_destroy(&server->connQueueMutex);
    ArrayFree(server->threadPool);
    Free(server);
}

static void *
HttpServerWorkerThread(void *args)
{
    HttpServerWorkerThreadArgs *wArgs = (HttpServerWorkerThreadArgs *) args;
    HttpServer *server = wArgs->server;

    while (!server->stop)
    {
        Stream *fp;
        HttpServerContext *context;

        char *line = NULL;
        size_t lineSize = 0;
        ssize_t lineLen = 0;

        char *requestMethodPtr;
        char *pathPtr;
        char *requestPath;
        char *requestProtocol;

        HashMap *requestParams;
        ssize_t requestPathLen;

        ssize_t i = 0;
        HttpRequestMethod requestMethod;

        long firstRead;

        fp = DequeueConnection(server);

        if (!fp)
        {
            /* Block for 1 millisecond before continuing so we don't
             * murder the CPU if the queue is empty. */
            UtilSleepMillis(1);
            continue;
        }

        /* Get the first line of the request.
         * 
         * Every once in a while, we're too fast for the client. When this
         * happens, UtilGetLine() sets errno to EAGAIN. If we get
         * EAGAIN, then clear the error on the stream and try again
         * after a few ms. This is typically more than enough time for
         * the client to send data. */
        firstRead = UtilServerTs();
        while ((lineLen = UtilGetLine(&line, &lineSize, fp)) == -1
               && errno == EAGAIN)
        {
            StreamClearError(fp);

            /* If the server is stopped, or it's been a while, just
             * give up so we aren't wasting a thread on this client. */
            if (server->stop || (UtilServerTs() - firstRead) > 1000 * 30)
            {
                goto finish;
            }

            UtilSleepMillis(5);
        }

        if (lineLen == -1)
        {
            goto bad_request;
        }

        requestMethodPtr = line;
        for (i = 0; i < lineLen; i++)
        {
            if (line[i] == ' ')
            {
                line[i] = '\0';
                break;
            }
        }

        if (i == lineLen)
        {
            goto bad_request;
        }

        requestMethod = HttpRequestMethodFromString(requestMethodPtr);
        if (requestMethod == HTTP_METHOD_UNKNOWN)
        {
            goto bad_request;
        }

        pathPtr = line + i + 1;

        for (i = 0; i < (line + lineLen) - pathPtr; i++)
        {
            if (pathPtr[i] == ' ')
            {
                pathPtr[i] = '\0';
                break;
            }
        }

        requestPathLen = i;
        requestPath = Malloc(((requestPathLen + 1) * sizeof(char)));
        strcpy(requestPath, pathPtr);

        requestProtocol = &pathPtr[i + 1];
        line[lineLen - 2] = '\0';  /* Get rid of \r and \n */

        if (strcmp(requestProtocol, "HTTP/1.1") != 0 && strcmp(requestProtocol, "HTTP/1.0") != 0)
        {
            Free(requestPath);
            goto bad_request;
        }

        /* Find request params */
        for (i = 0; i < requestPathLen; i++)
        {
            if (requestPath[i] == '?')
            {
                break;
            }
        }

        requestPath[i] = '\0';
        requestParams = (i == requestPathLen) ? NULL : HttpParamDecode(requestPath + i + 1);

        context = HttpServerContextCreate(requestMethod, requestPath, requestParams, fp);
        if (!context)
        {
            Free(requestPath);
            goto internal_error;
        }

        context->requestHeaders = HttpParseHeaders(fp);
        if (!context->requestHeaders)
        {
            goto internal_error;
        }

        server->requestHandler(context, server->handlerArgs);

        HttpServerContextFree(context);
        fp = NULL;                 /* The above call will close this
                                    * Stream */
        goto finish;

internal_error:
        StreamPuts(fp, "HTTP/1.0 500 Internal Server Error\n");
        StreamPuts(fp, "Connection: close\n");
        goto finish;

bad_request:
        StreamPuts(fp, "HTTP/1.0 400 Bad Request\n");
        StreamPuts(fp, "Connection: close\n");
        goto finish;

finish:
        Free(line);
        if (fp)
        {
            StreamClose(fp);
        }
    }

    return NULL;
}

static void *
HttpServerEventThread(void *args)
{
    HttpServer *server = (HttpServer *) args;
    struct pollfd pollFds[1];
    Stream *fp;
    size_t i;

    server->isRunning = 1;
    server->stop = 0;

    pollFds[0].fd = server->sd;
    pollFds[0].events = POLLIN;

    for (i = 0; i < server->nThreads; i++)
    {
        HttpServerWorkerThreadArgs *workerThread = Malloc(sizeof(HttpServerWorkerThreadArgs));

        if (!workerThread)
        {
            /* TODO: Make the event thread return an error to the main
             * thread */
            return NULL;
        }

        workerThread->server = server;
        workerThread->id = i;

        if (pthread_create(&workerThread->thread, NULL, HttpServerWorkerThread, workerThread) != 0)
        {
            /* TODO: Make the event thread return an error to the main
             * thread */
            return NULL;
        }

        ArrayAdd(server->threadPool, workerThread);
    }

    while (!server->stop)
    {
        struct sockaddr_storage addr;
        socklen_t addrLen = sizeof(addr);
        int connFd;
        int pollResult;


        pollResult = poll(pollFds, 1, 500);

        if (pollResult < 0)
        {
            /* The poll either timed out, or was interrupted. */
            continue;
        }

        pthread_mutex_lock(&server->connQueueMutex);

        /* Don't even accept connections if the queue is full. */
        if (!QueueFull(server->connQueue))
        {
            connFd = accept(server->sd, (struct sockaddr *) & addr, &addrLen);

            if (connFd < 0)
            {
                pthread_mutex_unlock(&server->connQueueMutex);
                continue;
            }

#ifdef TLS_IMPL
            if (server->flags & HTTP_FLAG_TLS)
            {
                fp = TlsServerStream(connFd, server->tlsCrt, server->tlsKey);
            }
            else
            {
                fp = StreamFd(connFd);
            }
#else
            fp = StreamFd(connFd);
#endif

            if (!fp)
            {
                pthread_mutex_unlock(&server->connQueueMutex);
                close(connFd);
                continue;
            }

            QueuePush(server->connQueue, fp);
        }
        pthread_mutex_unlock(&server->connQueueMutex);
    }

    for (i = 0; i < server->nThreads; i++)
    {
        HttpServerWorkerThreadArgs *workerThread = ArrayGet(server->threadPool, i);

        pthread_join(workerThread->thread, NULL);
        Free(workerThread);
    }

    while ((fp = DequeueConnection(server)))
    {
        StreamClose(fp);
    }

    server->isRunning = 0;

    return NULL;
}

int
HttpServerStart(HttpServer * server)
{
    if (!server)
    {
        return 0;
    }

    if (server->isRunning)
    {
        return 1;
    }

    if (pthread_create(&server->socketThread, NULL, HttpServerEventThread, server) != 0)
    {
        return 0;
    }

    return 1;
}

void
HttpServerJoin(HttpServer * server)
{
    if (!server)
    {
        return;
    }

    pthread_join(server->socketThread, NULL);
}

void
HttpServerStop(HttpServer * server)
{
    if (!server)
    {
        return;
    }

    server->stop = 1;
}
