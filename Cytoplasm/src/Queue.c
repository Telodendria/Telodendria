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
#include <Queue.h>

#include <Memory.h>

struct Queue
{
    void **items;
    size_t size;
    size_t front;
    size_t rear;
};

Queue *
QueueCreate(size_t size)
{
    Queue *q;

    if (!size)
    {
        /* Can't have a queue of length zero */
        return NULL;
    }

    q = Malloc(sizeof(Queue));
    if (!q)
    {
        return NULL;
    }

    q->items = Malloc(size * sizeof(void *));
    if (!q->items)
    {
        Free(q);
        return NULL;
    }

    q->size = size;
    q->front = size + 1;
    q->rear = size + 1;

    return q;
}

void
QueueFree(Queue * q)
{
    if (q)
    {
        Free(q->items);
    }

    Free(q);
}

int
QueueFull(Queue * q)
{
    if (!q)
    {
        return 0;
    }

    return ((q->front == q->rear + 1) || (q->front == 0 && q->rear == q->size - 1));
}

int
QueueEmpty(Queue * q)
{
    if (!q)
    {
        return 0;
    }

    return q->front == q->size + 1;
}

int
QueuePush(Queue * q, void *element)
{
    if (!q || !element)
    {
        return 0;
    }

    if (QueueFull(q))
    {
        return 0;
    }

    if (q->front == q->size + 1)
    {
        q->front = 0;
    }

    if (q->rear == q->size + 1)
    {
        q->rear = 0;
    }
    else
    {
        q->rear = (q->rear + 1) % q->size;
    }

    q->items[q->rear] = element;

    return 1;
}

void *
QueuePop(Queue * q)
{
    void *element;

    if (!q)
    {
        return NULL;
    }

    if (QueueEmpty(q))
    {
        return NULL;
    }

    element = q->items[q->front];

    if (q->front == q->rear)
    {
        q->front = q->size + 1;
        q->rear = q->size + 1;
    }
    else
    {
        q->front = (q->front + 1) % q->size;
    }

    return element;
}

void *
QueuePeek(Queue * q)
{
    if (!q)
    {
        return NULL;
    }

    if (QueueEmpty(q))
    {
        return NULL;
    }

    return q->items[q->front];
}
