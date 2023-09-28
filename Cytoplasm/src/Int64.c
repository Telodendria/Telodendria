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

#include <Log.h>

#ifdef INT64_NATIVE
#define Int64Sign(x) ((int) (((UInt64) (x)) >> 63))
#else
#define Int64Sign(x) ((int) ((x).i[1] >> 31))
#endif

size_t
Int64Str(Int64 x, int base, char *out, size_t len)
{
    static const char symbols[] = "0123456789ABCDEF";

    size_t i = len - 1;
    size_t j = 0;

    int neg = Int64Sign(x);

    Int64 base64 = Int64Create(0, base);

    /* We only have symbols up to base 16 */
    if (base < 2 || base > 16)
    {
        return 0;
    }

    /*
     * This algorithm doesn't work on INT64_MIN.
     *
     * But it works on all other integers in the range, so we
     * just scoot the range in by one for now. It's a hack and
     * I'm not a huge fan of it, but this function is mostly
     * used in Json, which shouldn't have a range this large
     * anyway (Json is limited to -2^53 -> 2^53-1).
     *
     * Proper fixes are always welcome.
     */
    if (Int64Eq(x, Int64Create(0x80000000, 0x00000000)))
    {
        x = Int64Add(x, Int64Create(0, 1));
    }
#if 0
    else if (Int64Eq(x, Int64Create(0x7FFFFFFF, 0xFFFFFFFF)))
    {
        x = Int64Sub(x, Int64Create(0, 1));
    }
#endif

    if (base != 2 && base != 8 && base != 16 && neg)
    {
        x = Int64Neg(x);
    }

    do
    {
        Int64 mod = Int64Rem(x, base64);
        Int32 low = Int64Low(mod);

        out[i] = symbols[low];
        i--;
        x = Int64Div(x, base64);
    } while (Int64Gt(x, Int64Create(0, 0)));

    if (base != 2 && base != 8 && base != 16)
    {
        /*
         * Binary, octal, and hexadecimal are known to
         * be bit representations. Everything else (notably
         * decimal) should include the negative sign.
         */
        if (neg)
        {
            out[i] = '-';
            i--;
        }
    }

    while (++i < len)
    {
        out[j++] = out[i];
    }

    out[j] = '\0';

    return j;
}

#ifndef INT64_NATIVE

/* No native 64-bit support, add our own */

Int64
Int64Create(UInt32 high, UInt32 low)
{
    Int64 x;

    x.i[0] = low;
    x.i[1] = high;

    return x;
}

Int64
Int64Add(Int64 x, Int64 y)
{
    Int64 z = Int64Create(0, 0);
    int carry;

    z.i[0] = x.i[0] + y.i[0];
    carry = z.i[0] < x.i[0];
    z.i[1] = x.i[1] + y.i[1] + carry;

    return z;
}

Int64
Int64Sub(Int64 x, Int64 y)
{
    return Int64Add(x, Int64Neg(y));
}

Int64
Int64Mul(Int64 x, Int64 y)
{
    Int64 z = Int64Create(0, 0);

    int xneg = Int64Sign(x);
    int yneg = Int64Sign(y);

    if (xneg)
    {
        x = Int64Neg(x);
    }

    if (yneg)
    {
        y = Int64Neg(y);
    }

    /* while (y > 0) */
    while (Int64Gt(y, Int64Create(0, 0)))
    {
        /* if (y & 1 != 0) */
        if (Int64Neq(Int64And(y, Int64Create(0, 1)), Int64Create(0, 0)))
        {
            z = Int64Add(z, x);
        }

        x = Int64Sll(x, 1);
        y = Int64Sra(y, 1);
    }

    if (xneg != yneg)
    {
        z = Int64Neg(z);
    }

    return z;
}

typedef struct
{
    Int64 q;
    Int64 r;
} Int64Ldiv;

static Int64Ldiv
Int64LongDivision(Int64 n, Int64 d)
{
    Int64Ldiv o;

    int i;

    int nneg = Int64Sign(n);
    int dneg = Int64Sign(d);

    o.q = Int64Create(0, 0);
    o.r = Int64Create(0, 0);

    if (Int64Eq(d, Int64Create(0, 0)))
    {
        raise(SIGFPE);
        return o;
    }

    if (nneg)
    {
        n = Int64Neg(n);
    }

    if (dneg)
    {
        d = Int64Neg(d);
    }

    for (i = 63; i >= 0; i--)
    {
        Int64 bit = Int64And(Int64Sra(n, i), Int64Create(0, 1));

        o.r = Int64Sll(o.r, 1);
        o.r = Int64Or(o.r, bit);

        if (Int64Geq(o.r, d))
        {
            o.r = Int64Sub(o.r, d);
            o.q = Int64Or(o.q, Int64Sll(Int64Create(0, 1), i));
        }
    }

    if (nneg != dneg)
    {
        o.r = Int64Neg(o.r);
        o.q = Int64Neg(o.q);
    }

    return o;
}

Int64
Int64Div(Int64 x, Int64 y)
{
    return Int64LongDivision(x, y).q;
}

Int64
Int64Rem(Int64 x, Int64 y)
{
    return Int64LongDivision(x, y).r;
}

Int64
Int64Sll(Int64 x, int y)
{
    Int64 z;

    if (!y)
    {
        return x;
    }

    z = Int64Create(0, 0);

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

Int64
Int64Sra(Int64 x, int y)
{
    Int64 z;

    int neg = Int64Sign(x);

    if (!y)
    {
        return x;
    }

    z = Int64Create(0, 0);

    if (y < 32)
    {
        z.i[0] = (x.i[1] << (32 - y)) | (x.i[0] >> y);
        z.i[1] = x.i[1] >> y;
    }
    else
    {
        z.i[0] = x.i[1] >> (y - 32);
    }

    if (neg)
    {
        Int64 mask = Int64Create(0xFFFFFFFF, 0xFFFFFFFF);

        z = Int64Or(Int64Sll(mask, (64 - y)), z);
    }

    return z;
}

Int64
Int64And(Int64 x, Int64 y)
{
    return Int64Create(x.i[1] & y.i[1], x.i[0] & y.i[0]);
}

Int64
Int64Or(Int64 x, Int64 y)
{
    return Int64Create(x.i[1] | y.i[1], x.i[0] | y.i[0]);
}

Int64
Int64Xor(Int64 x, Int64 y)
{
    return Int64Create(x.i[1] ^ y.i[1], x.i[0] ^ y.i[0]);
}

Int64
Int64Not(Int64 x)
{
    return Int64Create(~(x.i[1]), ~(x.i[0]));
}

int
Int64Eq(Int64 x, Int64 y)
{
    return x.i[0] == y.i[0] && x.i[1] == y.i[1];
}

int
Int64Lt(Int64 x, Int64 y)
{
    int xneg = Int64Sign(x);
    int yneg = Int64Sign(y);

    if (xneg != yneg)
    {
        return xneg > yneg;
    }
    else
    {
        if (xneg)
        {
            /* Both negative */
            return x.i[1] > y.i[1] || (x.i[1] == y.i[1] && x.i[0] > y.i[0]);
        }
        else
        {
            /* Both positive */
            return x.i[1] < y.i[1] || (x.i[1] == y.i[1] && x.i[0] < y.i[0]);
        }
    }
}

int
Int64Gt(Int64 x, Int64 y)
{
    int xneg = Int64Sign(x);
    int yneg = Int64Sign(y);

    if (xneg != yneg)
    {
        return xneg < yneg;
    }
    else
    {
        if (xneg)
        {
            /* Both negative */
            return x.i[1] < y.i[1] || (x.i[1] == y.i[1] && x.i[0] < y.i[0]);
        }
        else
        {
            /* Both positive */
            return x.i[1] > y.i[1] || (x.i[1] == y.i[1] && x.i[0] > y.i[0]);
        }
    }

}

#endif
