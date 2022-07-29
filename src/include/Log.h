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
 * Log.h: A heavily-modified version of Shlog, a simple C logging
 * facility that allows for colorful output, timestamps, and custom
 * log levels. This library differs from Shlog in that the naming
 * conventions have been updated to be consistent with Telodendria.
 *
 * Shlog was originally a learning project. It worked well, however,
 * and produced elegant logging output, so it was chosen to be the
 * main logging mechanism of Telodendria. The original Shlog project
 * is now dead; Shlog lives on now only as Telodendria's logging
 * mechanism.
 *
 * In the name of simplicity and portability, I opted to use an
 * in-house logging system instead of syslog(), or other system logging
 * mechanisms. However, this API could easily be patched to allow
 * logging via other mechanisms that support the same features.
 */
#ifndef TELODENDRIA_LOG_H
#define TELODENDRIA_LOG_H

#include <stdio.h>
#include <stddef.h>

/*
 * There are five log "levels," each one showing more information than
 * the previous one. A level of LOG_ERROR shows only errors, while a
 * level of LOG_DEBUG shows all output possible.
 */
typedef enum LogLevel
{
    LOG_ERROR,
    LOG_WARNING,
    LOG_TASK,
    LOG_MESSAGE,
    LOG_DEBUG
} LogLevel;

/*
 * The possible flags that can be applied to alter the behavior of
 * the logger
 */
typedef enum LogFlag
{
    LOG_FLAG_COLOR = (1 << 0)      /* Enable color output on TTYs */
} LogFlag;

/*
 * The log configurations structure in which all settings exist.
 * It's not super elegant to pass around a pointer to the logging
 * configuration
 */
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
