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
#include <Rand.h>

#include <Int.h>
#include <Util.h>
#include <Memory.h>

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define RAND_STATE_VECTOR_LENGTH 624
#define RAND_STATE_VECTOR_M 397

#define RAND_UPPER_MASK 0x80000000
#define RAND_LOWER_MASK 0x7FFFFFFF
#define RAND_TEMPER_B 0x9D2C5680
#define RAND_TEMPER_C 0xEFC60000

typedef struct RandState
{
    UInt32 mt[RAND_STATE_VECTOR_LENGTH];
    int index;
} RandState;

static void
RandSeed(RandState * state, UInt32 seed)
{
    state->mt[0] = seed & 0xFFFFFFFF;

    for (state->index = 1; state->index < RAND_STATE_VECTOR_LENGTH; state->index++)
    {
        state->mt[state->index] = (6069 * state->mt[state->index - 1]) & 0xFFFFFFFF;
    }
}

static UInt32
RandGenerate(RandState * state)
{
    static const UInt32 mag[2] = {0x0, 0x9908B0DF};

    UInt32 result;

    if (state->index >= RAND_STATE_VECTOR_LENGTH || state->index < 0)
    {
        int kk;

        if (state->index >= RAND_STATE_VECTOR_LENGTH + 1 || state->index < 0)
        {
            RandSeed(state, 4357);
        }

        for (kk = 0; kk < RAND_STATE_VECTOR_LENGTH - RAND_STATE_VECTOR_M; kk++)
        {
            result = (state->mt[kk] & RAND_UPPER_MASK) | (state->mt[kk + 1] & RAND_LOWER_MASK);
            state->mt[kk] = state->mt[kk + RAND_STATE_VECTOR_M] ^ (result >> 1) ^ mag[result & 0x1];
        }

        for (; kk < RAND_STATE_VECTOR_LENGTH - 1; kk++)
        {
            result = (state->mt[kk] & RAND_UPPER_MASK) | (state->mt[kk + 1] & RAND_LOWER_MASK);
            state->mt[kk] = state->mt[kk + (RAND_STATE_VECTOR_M - RAND_STATE_VECTOR_LENGTH)] ^ (result >> 1) ^ mag[result & 0x1];
        }

        result = (state->mt[RAND_STATE_VECTOR_LENGTH - 1] & RAND_UPPER_MASK) | (state->mt[0] & RAND_LOWER_MASK);
        state->mt[RAND_STATE_VECTOR_LENGTH - 1] = state->mt[RAND_STATE_VECTOR_M - 1] ^ (result >> 1) ^ mag[result & 0x1];
        state->index = 0;
    }

    result = state->mt[state->index++];
    result ^= (result >> 11);
    result ^= (result << 7) & RAND_TEMPER_B;
    result ^= (result << 15) & RAND_TEMPER_C;
    result ^= (result >> 18);

    return result;
}

static void
RandDestructor(void *p)
{
    Free(p);
}

/* Generate random numbers using rejection sampling. The basic idea is
 * to "reroll" if a number happens to be outside the range. However
 * this could be extremely inefficient.
 * 
 * Another idea would just be to "reroll" if the generated number ends up
 * in the previously "biased" range, and THEN do a modulo.
 * 
 * This would be far more efficient for small values of max, and fixes the
 * bias issue. */

/* This algorithm therefore computes N random numbers generally in O(N)
 * time, while being less biased. */
void
RandIntN(int *buf, size_t size, unsigned int max)
{
    static pthread_key_t stateKey;
    static int createdKey = 0;

    /* Limit the range to banish all previously biased results */
    const int allowed = RAND_MAX - RAND_MAX % max;

    RandState *state;
    int tmp;
    size_t i;

    if (!createdKey)
    {
        pthread_key_create(&stateKey, RandDestructor);
        createdKey = 1;
    }

    state = pthread_getspecific(stateKey);

    if (!state)
    {
        /* Generate a seed from the system time, PID, and TID */
        UInt32 seed = UtilServerTs() ^ getpid() ^ (unsigned long) pthread_self();

        state = Malloc(sizeof(RandState));
        RandSeed(state, seed);

        pthread_setspecific(stateKey, state);
    }

    /* Generate {size} random numbers. */
    for (i = 0; i < size; i++)
    {
        /* Most of the time, this will take about 1 loop */
        do
        {
            tmp = RandGenerate(state);
        } while (tmp > allowed);

        buf[i] = tmp % max;
    }
}

/* Generate just 1 random number */
int
RandInt(unsigned int max)
{
    int val = 0;

    RandIntN(&val, 1, max);
    return val;
}
