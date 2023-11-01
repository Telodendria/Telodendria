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
#ifndef TELODENDRIA_H
#define TELODENDRIA_H

/***
 * @Nm Telodendria
 * @Nd Branding and callback functions specific to Telodendria.
 * @Dd March 12 2023
 * @Xr Memory Log
 *
 * This API provides the callbacks used to hook Telodendria into the
 * various other APIs. It exists primarily to be called by
 * .Fn main ,
 * but these functions are not static so
 * .Fn main
 * can be in a separate compilation unit.
 */

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Log.h>
#include <Cytoplasm/HttpRouter.h>

#define TELODENDRIA_LOGO_WIDTH 56
#define TELODENDRIA_LOGO_HEIGHT 22

/**
 * This holds the Telodendria ASCII art logo.
 * .Va TELODENDRIA_LOGO_HEIGHT
 * and
 * .Va TELODENDRIA_LOGO_WIDTH
 * are the sizes of each dimension of the two-dimensional array.
 * .Va TELODENDRIA_LOGO_HEIGHT
 * is the number of lines the logo contains, and
 * .Va TELODENDRIA_LOGO_WIDTH
 * is the number of characters in each line.
 * .Pp
 * .Sy NOTE:
 * the Telodendria logo belongs solely to the Telodendria project. If
 * this code is modified and distributed as something other than a
 * packaging of the official Telodendria source package, the logo
 * must be replaced with a different one, or removed entirely. Consult
 * the licensing section of
 * .Xr telodendria 7
 * for details.
 */
extern const char
TelodendriaLogo[TELODENDRIA_LOGO_HEIGHT][TELODENDRIA_LOGO_WIDTH];

#define TELODENDRIA_HEADER_WIDTH 56
#define TELODENDRIA_HEADER_HEIGHT 6

/**
 * This holds the Telodendria ASCII art header. It follows the same
 * conventions as the logo, but is not under any particular
 * restrictions other than those spelled out in the license.
 */
extern const char
TelodendriaHeader[TELODENDRIA_HEADER_HEIGHT][TELODENDRIA_HEADER_WIDTH];

/**
 * This function follows the function prototype required by
 * .Fn MemoryHook .
 * It is executed every time an allocation, re-allocation, or free
 * occurs, and is responsible for logging memory operations to the
 * log.
 */
extern void TelodendriaMemoryHook(MemoryAction, MemoryInfo *, void *);

/**
 * Print the logo and header, along with the copyright year and holder,
 * and the version number, out to the global log.
 */
extern void TelodendriaPrintHeader(void);

#endif
