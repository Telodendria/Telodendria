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

#ifndef TELODENDRIA_LOG_H
#define TELODENDRIA_LOG_H

#include <stdio.h>
#include <stddef.h>
#include <syslog.h>

#include <Stream.h>

#define LOG_FLAG_COLOR  (1 << 0)
#define LOG_FLAG_SYSLOG (1 << 1)

typedef struct LogConfig LogConfig;

extern LogConfig *
 LogConfigCreate(void);

extern LogConfig *
 LogConfigGlobal(void);

extern void
 LogConfigFree(LogConfig *);

extern void
 LogConfigLevelSet(LogConfig *, int);

extern void
 LogConfigIndent(LogConfig *);

extern void
 LogConfigUnindent(LogConfig *);

extern void
 LogConfigIndentSet(LogConfig *, size_t);

extern void
 LogConfigOutputSet(LogConfig *, Stream *);

extern void
 LogConfigFlagSet(LogConfig *, int);

extern void
 LogConfigFlagClear(LogConfig *, int);

extern void
 LogConfigTimeStampFormatSet(LogConfig *, char *);

extern void
 Logv(LogConfig *, int, const char *, va_list);

extern void
 LogTo(LogConfig *, int, const char *,...);

extern void
 Log(int, const char *, ...);

#endif
