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
#ifndef TELODENDRIA_INT_H
#define TELODENDRIA_INT_H

#include <limits.h>

#define BIT64_MAX 18446744073709551615
#define BIT32_MAX 4294967295
#define BIT16_MAX 65535
#define BIT8_MAX 255

#ifndef UCHAR_MAX
#error Size of char data type is unknown. Define UCHAR_MAX.
#endif

#ifndef USHRT_MAX
#error Size of short data type is unknown. Define USHRT_MAX.
#endif

#ifndef UINT_MAX
#error Size of int data type is unknown. Define UINT_MAX.
#endif

#ifndef ULONG_MAX
#error Size of long data type is unknown. Define ULONG_MAX.
#endif

#if UCHAR_MAX == BIT8_MAX
typedef signed char Int8;
typedef unsigned char UInt8;

#else
#error Unable to determine suitable data type for 8-bit integers.
#endif

#if UINT_MAX == BIT16_MAX
typedef signed int Int16;
typedef unsigned int UInt16;

#elif USHRT_MAX == BIT16_MAX
typedef signed short Int16;
typedef unsigned short UInt16;

#elif UCHAR_MAX == BIT16_MAX
typedef signed char Int16;
typedef unsigned char UInt16;

#else
#error Unable to determine suitable data type for 16-bit integers.
#endif

#if ULONG_MAX == BIT32_MAX
typedef signed long Int32;
typedef unsigned long UInt32;

#elif UINT_MAX == BIT32_MAX
typedef signed int Int32;
typedef unsigned int UInt32;

#elif USHRT_MAX == BIT32_MAX
typedef signed short Int32;
typedef unsigned short UInt32;

#elif UCHAR_MAX == BIT32_MAX
typedef signed char Int32;
typedef unsigned char UInt32;

#else
#error Unable to determine suitable data type for 32-bit integers.
#endif

/* The ANSI C standard only guarantees a data size of up to 32 bits. */

#endif
