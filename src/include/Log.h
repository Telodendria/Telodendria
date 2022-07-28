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
#ifndef TELODENDRIA_LOG_H
#define TELODENDRIA_LOG_H

#include <stdio.h>
#include <stddef.h>

typedef enum LogLevel
{
    LOG_ERROR,
    LOG_WARNING,
    LOG_TASK,
    LOG_MESSAGE,
    LOG_DEBUG
} LogLevel;

typedef enum LogFlag
{
    LOG_FLAG_COLOR = (1 << 0)
} LogFlag;

typedef struct LogConfig LogConfig;

extern LogConfig *
 LogConfigCreate(void);

extern void
 LogConfigFree(LogConfig * config);

extern void
 LogConfigLevelSet(LogConfig * config, LogLevel level);

extern LogLevel
 LogConfigLevelGet(LogConfig * config);

extern void
 LogConfigIndentSet(LogConfig * config, size_t indent);

extern size_t
 LogConfigIndentGet(LogConfig * config);

extern void
 LogConfigIndent(LogConfig * config);

extern void
 LogConfigUnindent(LogConfig * config);

extern void
 LogConfigOutputSet(LogConfig * config, FILE * out);

extern void
 LogConfigFlagSet(LogConfig * config, int flags);

extern void
 LogConfigFlagClear(LogConfig * config, int flags);

extern int
 LogConfigFlagGet(LogConfig * config, int flags);

extern void
 LogConfigTimeStampFormatSet(LogConfig * config, char *tsFmt);

extern void
 Log(LogConfig * config, LogLevel level, const char *msg,...);

#endif
