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
 * included in all copies or substantial portions of the Software.
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
#ifndef TELODENDRIA_CONFIG_H
#define TELODENDRIA_CONFIG_H

#include <stdio.h>

#include <HashMap.h>
#include <Array.h>

typedef struct ConfigDirective ConfigDirective;

typedef struct ConfigParseResult ConfigParseResult;

extern ConfigParseResult *
 ConfigParse(FILE * stream);

extern unsigned int
 ConfigParseResultOk(ConfigParseResult * result);

extern size_t
 ConfigParseResultLineNumber(ConfigParseResult * result);

extern HashMap *
 ConfigParseResultGet(ConfigParseResult * result);

extern void
 ConfigParseResultFree(ConfigParseResult * result);

extern Array *
 ConfigValuesGet(ConfigDirective * directive);

extern HashMap *
 ConfigChildrenGet(ConfigDirective * directive);

extern void
 ConfigFree(HashMap * conf);

#endif                             /* TELODENDRIA_CONFIG_H */
