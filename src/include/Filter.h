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
#ifndef TELODENDRIA_FILTER_H
#define TELODENDRIA_FILTER_H

#include <Schema/Filter.h>

/***
 * @Nm Filter
 * @Nd Validate JSON filters and apply them to events.
 * @Dd June 17 2023
 *
 * The Matrix Client-Server API defines a mechanism for defining
 * filters and applying them to certain endpoints. This API allows
 * those filters to be validated and applied to events.
 */

/**
 * Apply the given filter to the given event, returning a
 * new event JSON with the filter applied, or NULL if the event
 * is excluded totally by the rules of the filter.
 */
extern HashMap *
FilterApply(Filter *, HashMap *);

#endif /* TELODENDRIA_FILTER_H */
