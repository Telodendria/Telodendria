/*
 * Copyright (C) 2022-2025 Jordan Bancino <@jordan:synapse.telodendria.org>
 * with other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

#ifndef TELODENDRIA_STATE_H
#define TELODENDRIA_STATE_H

/***
 * @Nm State
 * @Nd The Matrix state resolution algorithms.
 * @Dd July 6 2023
 *
 * .Nm implements the state resolution algorithms required
 * to maintain room state.
 */

#include <Cytoplasm/HashMap.h>

#include <Room.h>

/**
 * Retrieve the value of a state tuple.
 */
extern char *StateGet(HashMap *, char *, char *);

/**
 * Set a state tuple to a value.
 */
extern char *StateSet(HashMap *, char *, char *, char *);

/**
 * Compute the room state before the specified event was sent.
 */
extern HashMap * StateResolve(Room *, HashMap *);

#endif /* TELODENDRIA_STATE_H */
