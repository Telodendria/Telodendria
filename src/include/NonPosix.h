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
 * NonPosix.h: A collection of functions that I know certain operating
 * systems have, but they aren't specified in POSIX. I'd like to keep
 * this header really small if at all possible.
 */
#ifndef TELODENDRIA_NONPOSIX_H
#define TELODENDRIA_NONPOSIX_H

#include <stdarg.h>

/*
 * Pretty much all Unix-like systems have a chroot() function. In fact,
 * chroot() used to be POSIX, so any operating system claiming to be
 * POSIX-like had to have a chroot(). But unfortunately chroot() is not
 * in the standard anymore. Luckily, I don't know of a single operating
 * system to get rid of chroot(). So we should be able to safely depend
 * on a chroot() syscall being available.
 */
extern int chroot(const char *);

/*
 * Pretty much every syslog interface has a vsyslog(). Unfortuntately,
 * it is not POSIX.
 */
extern void vsyslog(int, const char *, va_list);

/*
 * Telodendria is primarily developed on OpenBSD; as such, you can
 * expect that it will use some OpenBSD-specific features if OpenBSD
 * is the target platform.
 *
 * It is my goal though, to make these functions entirely optional.
 * I've wrapped them in a preprocessor guard, and anywhere they're used
 * should also be wrapped in the same guard. So if the target platform
 * is not OpenBSD, then these aren't used at all.
 */
#ifdef __OpenBSD__
extern int pledge(const char *, const char *);
extern int unveil(const char *, const char *);
#endif

#endif
