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
#include <Sha2.h>
#include <Memory.h>
#include <Int.h>

#include <stdio.h>
#include <string.h>

#include <limits.h>

#define GET_UINT32(x) \
    (((UInt32)(x)[0] << 24) | \
     ((UInt32)(x)[1] << 16) | \
     ((UInt32)(x)[2] << 8) | \
     ((UInt32)(x)[3]))

#define PUT_UINT32(dst, x) { \
    (dst)[0] = (x) >> 24; \
    (dst)[1] = (x) >> 16; \
    (dst)[2] = (x) >> 8; \
    (dst)[3] = (x); \
}

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))

#define T0(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define T1(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))

#define CH(a, b, c) (((a) & (b)) ^ ((~(a)) & (c)))
#define MAJ(a, b, c) (((a) & (b)) ^ ((a) & (c)) ^ ((b) & (c)))
#define WW(i) (w[i] = w[i - 16] + S0(w[i - 15]) + w[i - 7] + S1(w[i - 2]))

#define ROUND(a, b, c, d, e, f, g, h, k, w) { \
    UInt32 tmp0 = h + T0(e) + CH(e, f, g) + k + w; \
    UInt32 tmp1 = T1(a) + MAJ(a, b, c); \
    h = tmp0 + tmp1; \
    d += tmp0; \
}

typedef struct Sha256Context
{
    size_t length;
    UInt32 state[8];
    size_t bufLen;
    unsigned char buffer[64];
} Sha256Context;

static void
Sha256Chunk(Sha256Context * context, unsigned char chunk[64])
{
    const UInt32 rk[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    UInt32 w[64];
    UInt32 a, b, c, d, e, f, g, h;

    int i;

    for (i = 0; i < 16; i++)
    {
        w[i] = GET_UINT32(&chunk[4 * i]);
    }

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    for (i = 0; i < 16; i += 8)
    {
        ROUND(a, b, c, d, e, f, g, h, rk[i], w[i]);
        ROUND(h, a, b, c, d, e, f, g, rk[i + 1], w[i + 1]);
        ROUND(g, h, a, b, c, d, e, f, rk[i + 2], w[i + 2]);
        ROUND(f, g, h, a, b, c, d, e, rk[i + 3], w[i + 3]);
        ROUND(e, f, g, h, a, b, c, d, rk[i + 4], w[i + 4]);
        ROUND(d, e, f, g, h, a, b, c, rk[i + 5], w[i + 5]);
        ROUND(c, d, e, f, g, h, a, b, rk[i + 6], w[i + 6]);
        ROUND(b, c, d, e, f, g, h, a, rk[i + 7], w[i + 7]);
    }

    for (i = 16; i < 64; i += 8)
    {
        ROUND(a, b, c, d, e, f, g, h, rk[i], WW(i));
        ROUND(h, a, b, c, d, e, f, g, rk[i + 1], WW(i + 1));
        ROUND(g, h, a, b, c, d, e, f, rk[i + 2], WW(i + 2));
        ROUND(f, g, h, a, b, c, d, e, rk[i + 3], WW(i + 3));
        ROUND(e, f, g, h, a, b, c, d, rk[i + 4], WW(i + 4));
        ROUND(d, e, f, g, h, a, b, c, rk[i + 5], WW(i + 5));
        ROUND(c, d, e, f, g, h, a, b, rk[i + 6], WW(i + 6));
        ROUND(b, c, d, e, f, g, h, a, rk[i + 7], WW(i + 7));
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void
Sha256Process(Sha256Context * context, unsigned char *data, size_t length)
{
    context->length += length;

    if (context->bufLen && context->bufLen + length >= 64)
    {
        int len = 64 - context->bufLen;

        memcpy(context->buffer + context->bufLen, data, len);
        Sha256Chunk(context, context->buffer);
        data += len;
        length -= len;
        context->bufLen = 0;
    }

    while (length >= 64)
    {
        Sha256Chunk(context, data);
        data += 64;
        length -= 64;
    }

    if (length)
    {
        memcpy(context->buffer + context->bufLen, data, length);
        context->bufLen += length;
    }
}

char *
Sha256(char *str)
{
    Sha256Context context;
    size_t i;
    unsigned char out[32];
    char *outStr;

    unsigned char fill[64];
    UInt32 fillLen;
    unsigned char buf[8];
    UInt32 hiLen;
    UInt32 loLen;

    if (!str)
    {
        return NULL;
    }

    outStr = Malloc(65);
    if (!outStr)
    {
        return NULL;
    }

    context.state[0] = 0x6a09e667;
    context.state[1] = 0xbb67ae85;
    context.state[2] = 0x3c6ef372;
    context.state[3] = 0xa54ff53a;
    context.state[4] = 0x510e527f;
    context.state[5] = 0x9b05688c;
    context.state[6] = 0x1f83d9ab;
    context.state[7] = 0x5be0cd19;

    context.bufLen = 0;
    context.length = 0;
    memset(context.buffer, 0, 64);

    Sha256Process(&context, (unsigned char *) str, strlen(str));

    memset(fill, 0, 64);
    fill[0] = 0x80;

    fillLen = (context.bufLen < 56) ? 56 - context.bufLen : 120 - context.bufLen;
    hiLen = (UInt32) (context.length >> 29);
    loLen = (UInt32) (context.length << 3);

    PUT_UINT32(&buf[0], hiLen);
    PUT_UINT32(&buf[4], loLen);

    Sha256Process(&context, fill, fillLen);
    Sha256Process(&context, buf, 8);

    for (i = 0; i < 8; i++)
    {
        PUT_UINT32(&out[4 * i], context.state[i]);
    }

    /* Convert to string */
    for (i = 0; i < 32; i++)
    {
        snprintf(outStr + (2 * i), 3, "%02x", out[i]);
    }

    return outStr;
}
