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
#include <Int64.h>

#include <stddef.h>
#include <signal.h>

size_t
UInt64Str(UInt64 x, int base, char *out, size_t len)
{
    static const char symbols[] = "0123456789ABCDEF";

    size_t i = len - 1;
    size_t j = 0;

    UInt64 base64 = UInt64Create(0, base);

    /* We only have symbols up to base 16 */
    if (base < 2 || base > 16)
    {
        return 0;
    }

    do
    {
        UInt64 mod = UInt64Rem(x, base64);
        UInt32 low = UInt64Low(mod);

        out[i] = symbols[low];
        i--;
        x = UInt64Div(x, base64);
    } while (UInt64Gt(x, UInt64Create(0, 0)));

    while (++i < len)
    {
        out[j++] = out[i];
    }

    out[j] = '\0';

    return j;
}

#ifndef UINT64_NATIVE

/* No native 64-bit support, add our own */

UInt64
UInt64Create(UInt32 high, UInt32 low)
{
    UInt64 x;

    x.i[0] = low;
    x.i[1] = high;

    return x;
}

UInt64
UInt64Add(UInt64 x, UInt64 y)
{
    UInt64 z = UInt64Create(0, 0);
    int carry;

    z.i[0] = x.i[0] + y.i[0];
    carry = z.i[0] < x.i[0];
    z.i[1] = x.i[1] + y.i[1] + carry;

    return z;
}

UInt64
UInt64Sub(UInt64 x, UInt64 y)
{
    UInt64 twosCompl = UInt64Add(UInt64Not(y), UInt64Create(0, 1));

    return UInt64Add(x, twosCompl);
}

UInt64
UInt64Mul(UInt64 x, UInt64 y)
{
    UInt64 z = UInt64Create(0, 0);

    /* while (y > 0) */
    while (UInt64Gt(y, UInt64Create(0, 0)))
    {
        /* if (y & 1 != 0) */
        if (UInt64Neq(UInt64And(y, UInt64Create(0, 1)), UInt64Create(0, 0)))
        {
            z = UInt64Add(z, x);
        }

        x = UInt64Sll(x, 1);
        y = UInt64Srl(y, 1);
    }

    return z;
}

typedef struct
{
    UInt64 q;
    UInt64 r;
} UInt64Ldiv;

static UInt64Ldiv
UInt64LongDivision(UInt64 n, UInt64 d)
{
    UInt64Ldiv o;

    int i;

    o.q = UInt64Create(0, 0);
    o.r = UInt64Create(0, 0);

    if (UInt64Eq(d, UInt64Create(0, 0)))
    {
        raise(SIGFPE);
        return o;
    }

    for (i = 63; i >= 0; i--)
    {
        UInt64 bit = UInt64And(UInt64Srl(n, i), UInt64Create(0, 1));
        o.r = UInt64Sll(o.r, 1);
        o.r = UInt64Or(o.r, bit);

        if (UInt64Geq(o.r, d))
        {
            o.r = UInt64Sub(o.r, d);
            o.q = UInt64Or(o.q, UInt64Sll(UInt64Create(0, 1), i));
        }
    }

    return o;
}

UInt64
UInt64Div(UInt64 x, UInt64 y)
{
    return UInt64LongDivision(x, y).q;
}

UInt64
UInt64Rem(UInt64 x, UInt64 y)
{
    return UInt64LongDivision(x, y).r;
}

UInt64
UInt64Sll(UInt64 x, int y)
{
    UInt64 z;

    if (!y)
    {
        return x;
    }

    z = UInt64Create(0, 0);

    if (y < 32)
    {
        z.i[1] = (x.i[0] >> (32 - y)) | (x.i[1] << y);
        z.i[0] = x.i[0] << y;
    }
    else
    {
        z.i[1] = x.i[0] << (y - 32);
    }

    return z;
}

UInt64
UInt64Srl(UInt64 x, int y)
{
    UInt64 z;

    if (!y)
    {
        return x;
    }

    z = UInt64Create(0, 0);

    if (y < 32)
    {
        z.i[0] = (x.i[1] << (32 - y)) | (x.i[0] >> y);
        z.i[1] = x.i[1] >> y;
    }
    else
    {
        z.i[0] = x.i[1] >> (y - 32);
    }

    return z;
}

UInt64
UInt64And(UInt64 x, UInt64 y)
{
    return UInt64Create(x.i[1] & y.i[1], x.i[0] & y.i[0]);
}

UInt64
UInt64Or(UInt64 x, UInt64 y)
{
    return UInt64Create(x.i[1] | y.i[1], x.i[0] | y.i[0]);
}

UInt64
UInt64Xor(UInt64 x, UInt64 y)
{
    return UInt64Create(x.i[1] ^ y.i[1], x.i[0] ^ y.i[0]);
}

UInt64
UInt64Not(UInt64 x)
{
    return UInt64Create(~(x.i[1]), ~(x.i[0]));
}

int
UInt64Eq(UInt64 x, UInt64 y)
{
    return x.i[0] == y.i[0] && x.i[1] == y.i[1];
}

int
UInt64Lt(UInt64 x, UInt64 y)
{
    return x.i[1] < y.i[1] || (x.i[1] == y.i[1] && x.i[0] < y.i[0]);
}

int
UInt64Gt(UInt64 x, UInt64 y)
{
    return x.i[1] > y.i[1] || (x.i[1] == y.i[1] && x.i[0] > y.i[0]);
}

#endif
