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
#include <HttpServer.h>

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct HttpServer
{
    int sd;
    unsigned int nThreads;
    HttpHandler *requestHandler;
    void *handlerArgs;

    pthread_t socketThread;

    volatile unsigned int stop:1;
    volatile unsigned int isRunning:1;
};

HttpServer *
HttpServerCreate(int socketDesc, unsigned int nThreads, HttpHandler * requestHandler, void *handlerArgs)
{
    HttpServer *server = malloc(sizeof(HttpServer));

    if (!server)
    {
        return NULL;
    }

    server->sd = socketDesc;
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
    free(server);
}

#include <stdio.h>

static void *
HttpServerEventThread(void *args)
{
    HttpServer *server = (HttpServer *) args;

    server->isRunning = 1;
    server->stop = 0;

    while (!server->stop)
    {
        printf("In server event thread\n");
        fflush(stdout);
        sleep(1);
    }

    server->isRunning = 0;

    printf("Event thread dying!\n");

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
