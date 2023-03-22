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

#ifndef TELODENDRIA_TELODENDRIACONFIG_H
#define TELODENDRIA_TELODENDRIACONFIG_H

#include <Log.h>
#include <HashMap.h>

typedef enum TelodendriaConfigFlag
{
    TELODENDRIA_FEDERATION = (1 << 0),
    TELODENDRIA_REGISTRATION = (1 << 1),
    TELODENDRIA_LOG_COLOR = (1 << 2),
    TELODENDRIA_LOG_FILE = (1 << 3),
    TELODENDRIA_LOG_STDOUT = (1 << 4),
    TELODENDRIA_LOG_SYSLOG = (1 << 5)
} TelodendriaConfigFlag;

typedef struct TelodendriaConfig
{
    char *serverName;
    char *baseUrl;
    char *identityServer;

    char *uid;
    char *gid;
    char *dataDir;

    unsigned short listenPort;
    unsigned int flags;
    unsigned int threads;
    unsigned int maxConnections;

    size_t maxCache;

    char *logTimestamp;
    int logLevel;
} TelodendriaConfig;

extern TelodendriaConfig *
 TelodendriaConfigParse(HashMap *);

extern void
 TelodendriaConfigFree(TelodendriaConfig *);

#endif
