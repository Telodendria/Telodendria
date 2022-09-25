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

#ifndef TELODENDRIA_ARRAY_H
#define TELODENDRIA_ARRAY_H

#include <stddef.h>

typedef struct Array Array;

extern Array *
 ArrayCreate(void);

extern size_t
 ArraySize(Array *);

extern void *
 ArrayGet(Array *, size_t);

extern int
 ArrayInsert(Array *, void *, size_t);

extern int
 ArrayAdd(Array *, void *);

extern void *
 ArrayDelete(Array *, size_t);

extern void
 ArraySort(Array *, int (*) (void *, void *));

extern void
 ArrayFree(Array *);

extern int
 ArrayTrim(Array *);

#endif                             /* TELODENDRIA_ARRAY_H */
