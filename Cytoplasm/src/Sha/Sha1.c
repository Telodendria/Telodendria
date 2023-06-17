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
#include <Sha.h>
#include <Memory.h>
#include <Int.h>

#include <string.h>

#include <limits.h>

#define LOAD32H(x, y) \
	{ \
		x = ((UInt32)((y)[0] & 255) << 24) | \
			((UInt32)((y)[1] & 255) << 16) | \
			((UInt32)((y)[2] & 255) <<  8) | \
			((UInt32)((y)[3] & 255));        \
	}

#define ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define BLK(i) (block->l[i & 15] = ROL(block->l[(i+13) & 15] ^ block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

#define R0(v, w, x, y, z, i) z += ((w & (x ^ y)) ^ y) + block->l[i] + 0x5A827999 + ROL(v, 5); w = ROL(w, 30);
#define R1(v, w, x, y, z, i) z += ((w & (x ^ y)) ^ y) + BLK(i) + 0x5A827999 + ROL(v, 5); w = ROL(w, 30);
#define R2(v, w, x, y, z, i) z += (w ^ x ^ y) + BLK(i) + 0x6ED9EBA1 + ROL(v, 5); w = ROL(w, 30);
#define R3(v, w, x, y, z, i) z += (((w | x) & y) | (w & x)) + BLK(i) + 0x8F1BBCDC + ROL(v, 5); w = ROL(w, 30);
#define R4(v, w, x, y, z, i) z += (w ^ x ^ y) + BLK(i) + 0xCA62C1D6 + ROL(v, 5); w = ROL(w, 30);

typedef union
{
	UInt8  c[64];
	UInt32 l[16];
} Char64Long16;

typedef struct
{
	UInt32 state[5];
	UInt32 count[2];
	UInt8  buffer[64];
} Sha1Context;

static void
Sha1Transform(UInt32 state[5], const UInt8 buffer[64])
{
	UInt32 a, b, c, d, e, i;
	UInt8 workspace[64];
	Char64Long16 *block = (Char64Long16 *) workspace;

	for (i = 0; i < 16; i++)
	{
		LOAD32H(block->l[i], buffer + (i * 4));
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

static void
Sha1Init(Sha1Context *ctx)
{
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xC3D2E1F0;

	ctx->count[0] = 0;
	ctx->count[1] = 0;
}

static void
Sha1Update(Sha1Context *ctx, const void *buf, UInt32 size)
{
	UInt32 i, j;

	j = (ctx->count[0] >> 3) & 63;

	if ((ctx->count[0] += size << 3) < (size << 3))
	{
		ctx->count[1]++;
	}

	ctx->count[1] += (size >> 29);

	if ((j + size) > 63)
	{
		i = 64 - j;

		memcpy(&ctx->buffer[j], buf, i);
		Sha1Transform(ctx->state, ctx->buffer);

		for ( ; i + 63 < size; i += 64)
		{
			Sha1Transform(ctx->state, (UInt8 *) buf + i);
		}

		j = 0;
	}
	else
	{
		i = 0;
	}

	memcpy(&ctx->buffer[j], &((UInt8 *)buf)[i], size - i);
}

static void
Sha1Calculate(Sha1Context *ctx, unsigned char *out)
{
	UInt32 i;
	UInt8 count[8];

	for (i = 0; i < 8; i++)
	{
		count[i] = (unsigned char) ((ctx->count[(i >= 4 ? 0 : 1)]
			>> ((3 - (i & 3)) * 8)) & 255);
	}

	Sha1Update(ctx, (UInt8 *) "\x80", 1);
	while ((ctx->count[0] & 504) != 448)
	{
		Sha1Update(ctx, (UInt8 *) "\0", 1);
	}

	Sha1Update(ctx, count, 8);
	for (i = 0; i < (160 / 8); i++)
	{
		out[i] = (UInt8) ((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
	}
}

unsigned char *
Sha1(char *str)
{
	Sha1Context ctx;
	unsigned char *out;

	if (!str)
	{
		return NULL;
	}

	out = Malloc(((160 / 8) + 1) * sizeof(unsigned char));
	if (!out)
	{
		return NULL;
	}

	Sha1Init(&ctx);
	Sha1Update(&ctx, str, strlen(str));
	Sha1Calculate(&ctx, out);

	out[160 / 8] = '\0';

	return out;
}
