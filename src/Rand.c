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

#include <Util.h>

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* Generate random numbers using rejection sampling.
* The basic idea is to "reroll" if a number happens to be
* outside the range. However this could be extremely inefficient.
*
* Another idea would just be to "reroll" if the generated number
* ends up in the previously "biased" range, and THEN do a modulo.
*
* This would be far more efficient for small values of max,
* and fixes the bias issue. */

/* This algorithm therefore computes N random numbers generally in 
 * O(N) time, while being less biased. */
void
RandIntN(int * buf, size_t size, unsigned int max)
{
    static pthread_mutex_t seedLock = PTHREAD_MUTEX_INITIALIZER;
    static unsigned int seed = 0;
    int tmp;
    /* Limit the range to banish all previously biased results */
    const int allowed = RAND_MAX - RAND_MAX % max;

    size_t i;

    pthread_mutex_lock(&seedLock);
    if (!seed)
    {
        /* Generate a seed from the system time, PID, and TID */
        seed = UtilServerTs() ^ getpid() ^ (unsigned long) pthread_self();
    }
    
    /* Generate {size} random numbers. */
    for (i = 0; i < size; i++)
    {
        /* Most of the time, this will take about 1 loop */
        do
        {
            tmp = rand_r(&seed);
        } while (tmp >= allowed);
        /* Since a generated number here is never in the biased range,
         * we can now safely use modulo. */
        buf[i] = tmp % max;
    }
    pthread_mutex_unlock(&seedLock);
}
/* Generate just 1 random number */
int
RandInt(unsigned int max)
{
    int val = 0;
    RandIntN(&val, 1, max);
    return val;
}
