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
#ifndef CYTOPLASM_QUEUE_H
#define CYTOPLASM_QUEUE_H

/***
 * @Nm Queue
 * @Nd A simple static queue data structure.
 * @Dd November 25 2022
 * @Xr Array HashMap
 *
 * .Nm
 * implements a simple queue data structure that is statically sized.
 * This implementation does not actually store the values of the items
 * in it; it only stores pointers to the data. As such, you will have
 * to manually maintain data and make sure it remains valid as long as
 * it is in the queue. The advantage of this is that
 * .Nm
 * doesn't have to copy data, and thus doesn't care how big the data
 * is. Furthermore, any arbitrary data can be stored in the queue.
 * .Pp
 * This queue implementation operates on the heap. It is a circular
 * queue, and it does not grow as it is used. Once the size is set,
 * the queue never gets any bigger.
 */

#include <stddef.h>

/**
 * These functions operate on a queue structure that is opaque to the
 * caller.
 */
typedef struct Queue Queue;

/**
 * Allocate a new queue that is able to store the specified number of
 * items in it.
 */
extern Queue * QueueCreate(size_t);

/**
 * Free the memory associated with the specified queue structure. Note
 * that this function does not free any of the values stored in the
 * queue; it is the caller's job to manage memory for each item.
 * Typically, the caller would dequeue all the items in the queue and
 * deal with them before freeing the queue itself.
 */
extern void QueueFree(Queue *);

/**
 * Push an element into the queue. This function returns a boolean
 * value indicating whether or not the push succeeded. Pushing items
 * into the queue will fail if the queue is full.
 */
extern int QueuePush(Queue *, void *);

/**
 * Pop an element out of the queue. This function returns NULL if the
 * queue is empty. Otherwise, it returns a pointer to the item that is
 * next up in the queue.
 */
extern void * QueuePop(Queue *);

/**
 * Retrieve a pointer to the item that is next up in the queue without
 * actually discarding it, such that the next call to
 * .Fn QueuePeek
 * or
 * .Fn QueuePop
 * will return the same pointer.
 */
extern void * QueuePeek(Queue *);

/**
 * Determine whether or not the queue is full.
 */
extern int QueueFull(Queue *);

/**
 * Determine whether or not the queue is empty.
 */
extern int QueueEmpty(Queue *);

#endif
