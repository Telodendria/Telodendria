/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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

/*
 * Util.h: Some misc. helper functions that provide functionality that
 * doesn't need its own full API. The functions here are entirely
 * stand-alone, and generally don't depend on any of the other APIs
 * defined by Telodendria.
 */
#ifndef TELODENDRIA_UTIL_H
#define TELODENDRIA_UTIL_H

/*
 * Get the current type in milliseconds since the Unix epoch. This uses
 * POSIX gettimeofday(2) and time_t, and converts it to a single number,
 * which is returned.
 *
 * A note on the 2038 problem: that's a long ways away, screw future
 * me!
 *
 * Kidding. As long as (sizeof(long) == 8), that is, as long as the
 * long datatype is 64 bits, which is is on all modern 64-bit Unix-like
 * operating systems, then everything should be fine. Expect
 * Telodendria on 32-bit machines to break in 2038. I didn't want to
 * try to hack together some system to store larger numbers than the
 * architecture supports. We can always re-evaluate things over the
 * next decade.
 *
 * Return: A long representing the current time in milliseconds since
 * the beginning of the Unix epoch, just as the Matrix spec requires.
 */
extern long
 UtilServerTs(void);

/*
 * Encode a single UTF-8 codepoint as a string buffer containing
 * between 1 and 4 bytes. The string buffer is allocated on the heap,
 * so it should be freed when it is no longer needed.
 *
 * Params:
 *
 *   (unsigned long) The UTF-8 codepoint to encode as a byte buffer.
 *
 * Return: a null-terminated byte buffer representing the UTF-8
 * codepoint.
 */
extern char *
 UtilUtf8Encode(unsigned long);


/*
 * Duplicate a null-terminated string, and return a new string on the
 * heap.
 *
 * Params:
 *   (char *) The string to duplicate. It can be located anywhere on
 *            the heap or the stack.
 *
 * Return: A pointer to a null-terminated string on the heap. You must
 * free() it when you're done with it. This may also return NULL if the
 * call to malloc() fails.
 */
extern char *
 UtilStringDuplicate(char *);

#endif                             /* TELODENDRIA_UTIL_H */
