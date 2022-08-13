/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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
#include <NonPosix.h>

#include <HttpServer.h>

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct HttpServer
{
    int sd;
    unsigned int nThreads;
    pthread_t socketThread;

    volatile unsigned int stop:1;
    volatile unsigned int isRunning:1;

    HttpHandler *requestHandler;
    void *handlerArgs;
};

HttpServer *
HttpServerCreate(unsigned short port, unsigned int nThreads,
                 HttpHandler * requestHandler, void *handlerArgs)
{
    HttpServer *server;
    struct sockaddr_in sa = {0};

    if (!requestHandler)
    {
        return NULL;
    }

    server = malloc(sizeof(HttpServer));
    if (!server)
    {
        return NULL;
    }

    server->sd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (server->sd < 0)
    {
        free(server);
        return NULL;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = port;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server->sd, (struct sockaddr *) & sa, sizeof(sa)) < 0)
    {
        close(server->sd);
        free(server);
        return NULL;
    }

    /* TODO: Make this a user-tunable parameter? */
    if (listen(server->sd, 32) < 0)
    {
        close(server->sd);
        free(server);
        return NULL;
    }

    server->nThreads = nThreads;
    server->requestHandler = requestHandler;
    server->handlerArgs = handlerArgs;
    server->stop = 0;
    server->isRunning = 0;

    return server;
}

void
HttpServerFree(HttpServer * server)
{
    if (!server)
    {
        return;
    }

    close(server->sd);
    free(server);
}

static void *
HttpServerEventThread(void *args)
{
    HttpServer *server = (HttpServer *) args;
    struct pollfd pollFds[1];

    server->isRunning = 1;
    server->stop = 0;

    pollFds[0].fd = server->sd;
    pollFds[0].events = POLLIN;

    while (!server->stop)
    {
        struct sockaddr_storage addr;
        socklen_t addrLen = sizeof(addr);
        int connFd;
        int pollResult = poll(pollFds, 1, 500);

        if (pollResult < 0)
        {
            /* The poll either timed out, or was interrupted. */
            continue;
        }

        connFd = accept(server->sd, (struct sockaddr *) & addr, &addrLen);

        if (connFd < 0)
        {
            continue;
        }

        close(connFd);
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