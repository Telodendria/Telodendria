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
#include <syslog.h>

/*
 * I used to define all my own constants, but now I use
 * those defined in syslog.h. Instead of replacing all the
 * references, I just map the old names to the new ones. If
 * you're ever bored one day, you can remove these, and then
 * go fix all the compiler errors that arise. Should be pretty
 * easy, just mind numbing.
 */
#define LOG_ERROR LOG_ERR
#define LOG_TASK LOG_NOTICE
#define LOG_MESSAGE LOG_INFO

/*
 * The possible flags that can be applied to alter the behavior of
 * the logger.
 */
typedef enum LogFlag
{
    LOG_FLAG_COLOR = (1 << 0),     /* Enable color output on TTYs */
    LOG_FLAG_SYSLOG = (1 << 1)     /* Log to the syslog instead of a
                                    * file */
} LogFlag;

/*
 * The log configurations structure in which all settings exist.
 * It's not super elegant to pass around a pointer to the logging
 * configuration, but this really is the best way without having a
 * global variable. It allows multiple loggers to exist if necessary,
 * and makes things more thread safe.
 */
typedef struct LogConfig LogConfig;

/*
 * Create a new log configuration on the heap. This will be passed to
 * every Log() call after it is configured.
 *
 * Return: A pointer to a new LogConfig that can be configured and used
 * for logging. It should have sane defaults; in other words, you should
 * be able to immediately start logging with it.
 */
extern LogConfig *
 LogConfigCreate(void);

/*
 * Free a log configuration. Future attempts to log with the passed
 * configuration will fail in an undefined way, such as by hanging the
 * process or segfaulting.
 *
 * Params:
 *
 *   (LogConfig *) The configuration to free. All memory associated with
 *                 configuring the logging mechanism will be
 *                 invalidated.
 */
extern void
 LogConfigFree(LogConfig *);

/*
 * Set the current log level on the specified log configuration. This
 * indicates that only messages at or above this level should be
 * logged; all other messages are ignored by the Log() function.
 *
 * Params:
 *
 *   (LogConfig *) The log configuration to set the log level on.
 *   (int)    The log level to set.
 */
extern void
 LogConfigLevelSet(LogConfig *, int);

/*
 * Indent the log output by two spaces. This can be helpful in
 * generating stack traces, or otherwise producing hierarchical output.
 * After calling this function, all future log messages using this
 * configuration will be indented.
 *
 * Params:
 *
 *   (LogConfig *) The log configuration to indent.
 */
extern void
 LogConfigIndent(LogConfig *);

/*
 * Decrease the log output indent by two spaces. This can be helpful in
 * generating stack traces, or otherwise producing hierarchical output.
 * After calling this function, all future log messages using this
 * configuration will be unindented, unless there was no indentation
 * to begin with; in that case, this function will do nothing.
 *
 * Params:
 *
 *   (LogConfig *) The log configuration to unindent.
 */
extern void
 LogConfigUnindent(LogConfig *);

/*
* Set the log output indent to an arbitrary amount. This can be helpful
* in generating stack traces, or otherwise producing hierarchical
* output. After calling this function, all future log messages using
* this configuration will be indented by the given amount.
*
* Params:
*
*   (LogConfig *) The log configuration to apply the indent to.
*/
extern void
 LogConfigIndentSet(LogConfig *, size_t);

extern void
 LogConfigOutputSet(LogConfig *, FILE *);

extern void
 LogConfigFlagSet(LogConfig *, int);

extern void
 LogConfigFlagClear(LogConfig *, int);

extern void
 LogConfigTimeStampFormatSet(LogConfig *, char *);

/*
 * Actually log a message to a console, file, or other output device,
 * using the given log configuration. This function is thread-safe; it
 * locks a mutex before writing a message, and then unlocks it after
 * the message was written. It should therefore work well in
 * multithreaded environments, and with multiple different log
 * configurations, as each one has its own mutex.
 *
 * This function only logs messages if they are above the currently
 * configured log level. In this way, it is easy to turn some messages
 * on and off.
 *
 * This function is a printf() style function; it takes a format
 * string and any number of parameters to format.
 *
 * Params:
 *
 *   (LogConfig *)  The logging configuration.
 *   (int)     The level of the message to log.
 *   (const char *) The format string, or a plain message string.
 *   (...)          Any items to map into the format string, printf()
 *                  style.
 */
extern void
 Log(LogConfig *, int, const char *,...);

#endif
