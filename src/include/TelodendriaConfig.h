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
 * TelodendriaConfig.h: Validate and maintain the Telodendria server's
 * configuration data. This API builds on the Config API to add
 * Telodendria-specific parsing. It takes a fully parsed Config, and
 * converts it into a TelodendriaConfig, which is more structured.
 */
#ifndef TELODENDRIA_TELODENDRIACONFIG_H
#define TELODENDRIA_TELODENDRIACONFIG_H

#include <Log.h>
#include <HashMap.h>

typedef enum TelodendriaConfigFlag
{
    TELODENDRIA_FEDERATION = (1 << 0),
    TELODENDRIA_REGISTRATION = (1 << 1),
    TELODENDRIA_LOG_COLOR = (1 << 2)
} TelodendriaConfigFlag;

/*
 * Since this configuration will live in memory for a long time, it is
 * important that unused values are freed as soon as possible. Therefore,
 * the TelodendriaConfig structure is not opaque; values are accessed
 * directly, and they can be freed as the program wishes.
 *
 * NOTE: If you're going to free a value, make sure you set the pointer
 * to NULL. TelodendriaConfigFree() will call free() on all values.
 */
typedef struct TelodendriaConfig
{
    char *listenHost;
    unsigned short listenPort;
    char *serverName;
    char *chroot;
    char *uid;
    char *gid;
    char *dataDir;

    unsigned int flags;
    unsigned int threads;

    char *logOut;
    char *logTimestamp;
    LogLevel logLevel;
} TelodendriaConfig;

/*
 * Parse a Config map, extracting the necessary values, validating them,
 * and then adding them to a new TelodendriaConfig for future use by the
 * program. All values are copied, so the Config hash map can be safely
 * freed if this function succeeds.
 *
 * Params:
 *   (HashMap *)   A hash map from ConfigParse(). This should be a map of
 *                 ConfigDirectives.
 *   (LogConfig *) A working log configuration. Messages are written to
 *                 this log as the parsing progresses, and this log is
 *                 copied into the resulting TelodendriaConfig. It is
 *                 also potentially modified by the configuration file's
 *                 "log" block.
 *
 * Return: A TelodendriaConfig that is completely independent of the passed
 * configuration hash map, or NULL if one or more required values is missing.
 */
extern TelodendriaConfig *
 TelodendriaConfigParse(HashMap *, LogConfig *);

/*
 * Free all of the memory allocated to the given configuration. This
 * function unconditionally calls free() on all items in the structure,
 * so make sure items that were already freed are NULL.
 *
 * Params:
 *   (TelodendriaConfig *) The configuration to free all the values for.
 */
extern void
 TelodendriaConfigFree(TelodendriaConfig *);

#endif
