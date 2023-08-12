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
#ifndef CYTOPLASM_INT64_H
#define CYTOPLASM_INT64_H

/***
 * @Nm Int64
 * @Nd Fixed-width 64 bit integers.
 * @Dd August 11, 2023
 *
 * .Pp
 * ANSI C89 (or C99 for that matter) provides no required mechanism
 * for 64 bit integers. Nevertheless, many compilers provide them as
 * extensions. However, since it is not a gaurantee, and to be fully
 * standards-compliant and thus portable, a platform-agnostic interface
 * is required. This header provides such an interface. If the platform
 * has a 64 bit integer type, that is used, and native operations are
 * performed by C preprocessor macro expansion. Otherwise, a
 * compatibility layer is provided, which implements 64-bit
 * arithmetic on an array of 2 32-bit numbers which are provided by
 * .Xr Int 3 .
 * .Pp
 * Note that 64-bit emulation is certainly not as performant as using
 * native 64-bit operations, so whenever possible, the native
 * operations should be preferred. However, since C provides no required
 * 64 bit integer on 32-bit and less platforms, this API can be used as
 * a "good enough" fallback mechanism.
 * .Pp
 * Also note that this implementation, both in the native and
 * non-native forms, makes some assumptions:
 * .Bl -bullet -width Ds
 * .It
 * When a cast from a larger integer to a smaller integer is performed,
 * the upper bits are truncated, not the lower bits.
 * .It
 * Negative numbers are represented in memory and in registers in two's
 * compliment form.
 * .El
 * .Pp
 * This API may provide unexpected output if these assumptions are
 * false for a given platform.
 *
 * @ignore-typedefs
 */

#include <Int.h>

#include <stddef.h>

#define BIT64_MAX 18446744073709551615UL

/* TODO: Implement signed arithmetic */

#if UINT_MAX == BIT64_MAX
/* typedef signed int Int64; */
typedef unsigned int UInt64;

#define UINT64_NATIVE

#elif ULONG_MAX == BIT64_MAX
/* typedef signed int Int64; */
typedef unsigned long UInt64;

#define UINT64_NATIVE

#endif

#ifdef UINT64_NATIVE

#define UInt64Create(high, low) (((UInt64) (high) << 32) | (low))
#define UInt64Low(a) ((UInt32) ((a) & 0x00000000FFFFFFFF))
#define UInt64High(a) ((UInt32) ((a) >> 32))

#define UInt64Add(a, b) ((a) + (b))
#define UInt64Sub(a, b) ((a) - (b))
#define UInt64Mul(a, b) ((a) * (b))
#define UInt64Div(a, b) ((a) / (b))
#define UInt64Rem(a, b) ((a) % (b))

#define UInt64Bsl(a, b) ((a) << (b))
#define UInt64Bsr(a, b) ((a) >> (b))

#define UInt64And(a, b) ((a) & (b))
#define UInt64Or(a, b) ((a) | (b))
#define UInt64Xor(a, b) ((a) ^ (b))
#define UInt64Not(a) (~(a))

#define UInt64Eq(a, b) ((a) == (b))
#define UInt64Lt(a, b) ((a) < (b))
#define UInt64Gt(a, b) ((a) > (b))

#define UInt64Neq(a, b) ((a) != (b))
#define UInt64Leq(a, b) ((a) <= (b))
#define UInt64Geq(a, b) ((a) >= (b))

#else

/**
 * For platforms that do not have a native integer large enough to
 * store a 64 bit integer, this struct is used. i[0] contains the low
 * bits of integer, and i[1] contains the high bits of the integer.
 * .Pp
 * This struct should not be accessed directly, because UInt64 may not
 * actually be this struct, it might be an actual integer type. For
 * maximum portability, only use the functions defined here to
 * manipulate 64 bit integers.
 */
typedef struct
{
    UInt32 i[2];
} UInt64;

/**
 * Create a new unsigned 64 bit integer using the given high and low
 * bits.
 */
extern UInt64 UInt64Create(UInt32, UInt32);

/**
 * Add two unsigned 64 bit integers together.
 */
extern UInt64 UInt64Add(UInt64, UInt64);

/**
 * Subtract the second 64 bit integer from the first.
 */
extern UInt64 UInt64Sub(UInt64, UInt64);

/**
 * Multiply two 64 bit integers together. The non-native version of
 * this function uses the Russian Peasant method of multiplication,
 * which should afford more performance than a naive multiplication by
 * addition, but it is still rather slow and depends on the size of
 * the integers being multiplied.
 */
extern UInt64 UInt64Mul(UInt64, UInt64);

/**
 * Divide the first 64 bit integer by the second and return the
 * quotient. The non-native version of this function uses naive binary
 * long division, which is slow, but gauranteed to finish in constant
 * time.
 */
extern UInt64 UInt64Div(UInt64, UInt64);

/**
 * Divide the first 64 bit integer by the second and return the
 * remainder. The non-native version of this function uses naive binary
 * long division, which is slow, but gauranteed to finish in constant
 * time.
 */
extern UInt64 UInt64Rem(UInt64, UInt64);

/**
 * Perform a left logical bit shift of a 64 bit integer. The second
 * parameter is how many places to shift, and is declared as a regular
 * integer because anything more than 64 does not make sense.
 */
extern UInt64 UInt64Sll(UInt64, int);

/**
 * Perform a right logical bit shift of a 64 bit integer. The second
 * parameter is how many places to shift, and is declared as a regular
 * integer because anything more than 64 does not make sense.
 */
extern UInt64 UInt64Srl(UInt64, int);

/**
 * Perform a bitwise AND (&) of the provided 64 bit integers.
 */
extern UInt64 UInt64And(UInt64, UInt64);

/**
 * Perform a bitwise OR (|) of the provided 64 bit integers.
 */
extern UInt64 UInt64Or(UInt64, UInt64);

/**
 * Perform a bitwise XOR (^) of the provided 64 bit integers.
 */
extern UInt64 UInt64Xor(UInt64, UInt64);

/**
 * Perform a bitwise NOT (~) of the provided 64 bit integer.
 */
extern UInt64 UInt64Not(UInt64);

/**
 * Perform a comparison of the provided 64 bit integers and return a C
 * boolean that is true if and only if they are equal.
extern int UInt64Eq(UInt64, UInt64);

/**
 * Perform a comparison of the provided 64 bit integers and return a C
 * boolean that is true if and only if the second operand is strictly
 * less than the first.
 */
extern int UInt64Lt(UInt64, UInt64);

/**
 * Perform a comparison of the provided 64 bit integers and return a C
 * boolean that is true if and only if the second operand is strictly
 * greater than the first.
 */
extern int UInt64Gt(UInt64, UInt64);

#define UInt64Low(a) ((a).i[0])
#define UInt64High(a) ((a).i[1])

#define UInt64Neq(a, b) (!UInt64Eq(a, b))
#define UInt64Leq(a, b) (UInt64Eq(a, b) || UInt64Lt(a, b))
#define UInt64Geq(a, b) (UInt64Eq(a, b) || UInt64Gt(a, b))

#endif

#define UINT64_STRBUF 65 /* Base 2 representation with '\0' */

/**
 * Convert a 64 bit integer to a string in an arbitrary base
 * representation specified by the second parameter, using the provided
 * buffer and length specified by the third and fourth parameters. To
 * guarantee that the string will fit in the buffer, allocate it of
 * size UINT64_STRBUF or larger. Note that a buffer size smaller than
 * UINT64_STRBUF will invoke undefined behavior.
 */
extern size_t UInt64Str(UInt64, int, char *, size_t);

#endif /* CYTOPLASM_INT64_H */
