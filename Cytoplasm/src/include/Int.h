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
#ifndef CYTOPLASM_INT_H
#define CYTOPLASM_INT_H

/***
 * @Nm Int
 * @Nd Fixed-width integer types.
 * @Dd April 27 2023
 *
 * This header provides cross-platform, fixed-width integer types.
 * Specifically, it uses preprocessor magic to define the following
 * types:
 * .Bl -bullet -offset indent
 * .It
 * Int8 and UInt8
 * .It
 * Int16 and UInt16
 * .It
 * Int32 and UInt32
 * .El
 * .Pp
 * Note that there is no 64-bit integer type, because the ANSI C
 * standard makes no guarantee that such a type will exist, even
 * though it does on most platforms.
 * .Pp
 * The reason Cytoplasm provides its own header for this is
 * because ANSI C does not define fixed-width types, and while it
 * should be safe to rely on C99 fixed-width types in most cases,
 * there may be cases where even that is not possible.
 *
 * @ignore-typedefs
 */

#include <limits.h>

#define BIT32_MAX 4294967295UL
#define BIT16_MAX 65535UL
#define BIT8_MAX 255UL

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

#endif /* CYTOPLASM_INT_H */
