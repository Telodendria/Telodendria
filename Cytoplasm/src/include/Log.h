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

#ifndef CYTOPLASM_LOG_H
#define CYTOPLASM_LOG_H

/***
 * @Nm Log
 * @Nd A simple logging framework for logging to multiple destinations.
 * @Dd April 27 2023
 * @Xr Stream
 *
 * .Nm
 * is a simple C logging library that allows for colorful outputs,
 * timestamps, and custom log levels. It also features the ability to
 * have multiple logs open at one time, although Cytoplasm primarily
 * utilizes the global log. All logs are thread safe.
 */

#include <stdio.h>
#include <stddef.h>
#include <syslog.h>

#include <Stream.h>

#define LOG_FLAG_COLOR  (1 << 0)
#define LOG_FLAG_SYSLOG (1 << 1)

/**
 * A log is defined as a configuration that describes the properties
 * of the log. This opaque structure can be manipulated by the
 * functions defined in this API.
 */
typedef struct LogConfig LogConfig;

/**
 * Create a new log configuration with sane defaults that can be used
 * immediately with the logging functions.
 */
extern LogConfig * LogConfigCreate(void);

/**
 * Get the global log configuration, creating a new one with
 * .Fn LogConfigCreate
 * if necessary.
 */
extern LogConfig * LogConfigGlobal(void);

/**
 * Free the given log configuration. Note that this does not close the
 * underlying stream associated with the log, if there is one. Also
 * note that to avoid memory leaks, the global log configuration must
 * also be freed, but it cannot be used after it is freed.
 */
extern void LogConfigFree(LogConfig *);

/**
 * Set the current log level on the specified log configuration.
 * This indicates that only messages at or above this level should be
 * logged; all others are silently discarded. The passed log level
 * should be one of the log levels defined by
 * .Xr syslog 3 .
 * Refer to that page for a complete list of acceptable log levels,
 * and note that passing an invalid log level will result in undefined
 * behavior.
 */
extern void LogConfigLevelSet(LogConfig *, int);

/**
 * Cause the log output to be indented two more spaces than it was
 * previously. This can be helpful when generating stack traces or
 * other hierarchical output. This is a simple convenience wrapper
 * around
 * .Fn LogConfigIndentSet .
 */
extern void LogConfigIndent(LogConfig *);

/**
 * Cause the log output to be indented two less spaces than it was
 * previously. This is a simple convenience wrapper around
 * .Fn LogConfigIndentSet .
 */
extern void LogConfigUnindent(LogConfig *);

/**
 * Indent future log output using the specified config by some
 * arbitrary amount.
 */
extern void LogConfigIndentSet(LogConfig *, size_t);

/**
 * Set the file stream that logging output should be written to. This
 * defaults to standard output, but it can be set to standard error,
 * or any other arbitrary stream. Passing a NULL value for the stream
 * pointer sets the log output to the standard output. Note that the
 * output stream is only used if
 * .Va LOG_FLAG_SYSLOG
 * is not set.
 */
extern void LogConfigOutputSet(LogConfig *, Stream *);

/**
 * Set a number of boolean options on a log configuration. This
 * function uses bitwise operators, so multiple options can be set with
 * a single function call using bitwise OR operators. The flags are
 * defined as preprocessor macros, and are as follows:
 * .Bl -tag -width Ds
 * .It LOG_FLAG_COLOR
 * When set, enable color-coded output on TTYs. Note that colors are
 * implemented as ANSI escape sequences, and are not written to file
 * streams that are not actually connected to a TTY, to prevent those
 * sequences from being written to a file.
 * .Xr isatty 3
 * is checked before writing any terminal sequences.
 * .It LOG_FLAG_SYSLOG
 * When set, log output to the syslog using
 * .Xr syslog 3 ,
 * instead of logging to the file set by
 * .Fn LogConfigOutputSet .
 * This flag always overrides the stream set by that function,
 * regardless of when it was set, even if it was set after this flag
 * was set.
 * .El
 */
extern void LogConfigFlagSet(LogConfig *, int);

/**
 * Clear a boolean flag from the specified log format. See above for
 * the list of flags.
 */
extern void LogConfigFlagClear(LogConfig *, int);

/**
 * Set a custom timestamp to be prepended to each message if the
 * output is not going to the system log. Consult your system's
 * documentation for
 * .Xr strftime 3 .
 * A value of NULL disables the timestamp output before messages.
 */
extern void LogConfigTimeStampFormatSet(LogConfig *, char *);

/**
 * This function does the actual logging of messages using a
 * specified configuration. It takes the configuration, the log
 * level, a format string, and then a list of arguments, all in that
 * order. This function only logs messages if their level is above
 * or equal to the currently configured log level, making it easy to
 * turn some messages on or off.
 * .Pp
 * This function has the same usage as
 * .Xr vprintf 3 .
 * Consult that page for the list of format specifiers and their
 * arguments. This function is typically not used directly, see the
 * other log functions for the most common use cases.
 */
extern void Logv(LogConfig *, int, const char *, va_list);

/**
 * Log a message using
 * .Fn Logv .
 * with the specified configuration. This function has the same usage
 * as
 * .Xr printf 3 .
 */
extern void LogTo(LogConfig *, int, const char *, ...);

/**
 * Log a message to the global log using
 * .Fn Logv .
 * This function has the same usage as
 * .Xr printf 3 .
 */
extern void Log(int, const char *, ...);

#endif
