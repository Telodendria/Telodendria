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

#ifndef CYTOPLASM_RAND_H
#define CYTOPLASM_RAND_H

/***
 * @Nm Rand
 * @Nd Thread-safe random numbers.
 * @Dd February 16 2023
 * @Xr Util
 *
 * .Nm
 * is used for generating random numbers in a thread-safe way.
 * Currently, one generator state is shared across all threads, which
 * means that only one thread can generate random numbers at a time.
 * This state is protected with a mutex to guarantee this behavior.
 * In the future, a seed pool may be maintained to allow multiple
 * threads to generate random numbers at the same time.
 * .Pp
 * The generator state is seeded on the first call to a function that
 * needs it. The seed is determined by the current timestamp, the ID
 * of the process, and the thread ID. These should all be sufficiently
 * random sources, so the seed should be secure enough.
 * .Pp
 * .Nm
 * currently uses a simple Mersenne Twister algorithm to generate
 * random numbers. This algorithm was chosen because it is extremely
 * popular and widespread. While it is likely not cryptographically
 * secure, and does suffer some unfortunate pitfalls, this algorithm
 * has stood the test of time and is simple enough to implement, so
 * it was chosen over the alternatives.
 * .Pp
 * .Nm
 * does not use any random number generator functions from the C
 * standard library, since these are often flawed.
 */

#include <stddef.h>

/**
 * Generate a single random integer between 0 and the passed value.
 */
extern int RandInt(unsigned int);

/**
 * Generate the number of integers specified by the second argument
 * storing them into the buffer pointed to in the first argument.
 * Ensure that each number is between 0 and the third argument.
 * .Pp
 * This function allows a caller to get multiple random numbers at once
 * in a more efficient manner than repeatedly calling
 * .Fn RandInt ,
 * since each call to these functions
 * has to lock and unlock a mutex. It is therefore better to obtain
 * multiple random numbers in one pass if multiple are needed.
 */
extern void RandIntN(int *, size_t, unsigned int);

#endif                             /* CYTOPLASM_RAND_H */
