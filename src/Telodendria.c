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

#include <Telodendria.h>
#include <Memory.h>
#include <Log.h>

const char
 TelodendriaLogo[TELODENDRIA_LOGO_HEIGHT][TELODENDRIA_LOGO_WIDTH] = {
    "            .=                       -=-               ",
    "          :.:+                     .=:.                ",
    "         .=+-==.                  :.                   ",
    "           .+-                   =.                    ",
    "           .+                   :+.                    ",
    "            ==.                 -+:                    ",
    "             =++==--::           =+.                   ",
    "               .:::--=+=:        :+=                   ",
    "                       :==.      -=:                   ",
    "                         ===----=-.           ... :+.  ",
    "                       :==+=======:        .-+-::-+-=+=",
    "                      .==*%#=======       :+-      ..  ",
    "                 .:--=-===+=========-.   :+:           ",
    "              .=++=::..:============-+=-=-             ",
    ":+=:        :=+-:      .-=========-.  .                ",
    " =+++:  .:=+-:      .:--. .--:==:                      ",
    "   ::---:..       :=+:        ==                       ",
    "                  ++.        .+-                       ",
    "                  =+         .+-     ...:              ",
    "                  +-          -+-:-+=::+:              ",
    "        :=-....:-=:            .--:    =-              ",
    "     -++=:.:::..                                       "
};

const char
 TelodendriaHeader[TELODENDRIA_HEADER_HEIGHT][TELODENDRIA_HEADER_WIDTH] = {
    "=======================================================",
    "|_   _|__| | ___   __| | ___ _ __   __| |_ __(_) __ _  ",
    "  | |/ _ \\ |/ _ \\ / _` |/ _ \\ '_ \\ / _` | '__| |/ _` | ",
    "  | |  __/ | (_) | (_| |  __/ | | | (_| | |  | | (_| | ",
    "  |_|\\___|_|\\___/ \\__,_|\\___|_| |_|\\__,_|_|  |_|\\__,_| ",
    "======================================================="
};

void
TelodendriaHexDump(size_t off, char *hexBuf, char *asciiBuf, void *args)
{
    LogConfig *lc = args;

    if (hexBuf && asciiBuf)
    {
        Log(lc, LOG_DEBUG, "%04x: %s | %s |", off, hexBuf, asciiBuf);
    }
    else
    {
        Log(lc, LOG_DEBUG, "%04x", off);
    }
}

void
TelodendriaMemoryHook(MemoryAction a, MemoryInfo * i, void *args)
{
    LogConfig *lc = (LogConfig *) args;
    char *action;

    switch (a)
    {
        case MEMORY_ALLOCATE:
            action = "Allocated";
            break;
        case MEMORY_REALLOCATE:
            action = "Re-allocated";
            break;
        case MEMORY_FREE:
            action = "Freed";
            break;
        case MEMORY_BAD_POINTER:
            action = "Bad pointer to";
            break;
        default:
            action = "Unknown operation on";
            break;
    }

    Log(lc, a == MEMORY_BAD_POINTER ? LOG_WARNING : LOG_DEBUG,
        "%s:%d: %s %lu bytes of memory at %p.",
        MemoryInfoGetFile(i), MemoryInfoGetLine(i),
        action, MemoryInfoGetSize(i),
        MemoryInfoGetPointer(i));
}

void
TelodendriaMemoryIterator(MemoryInfo * i, void *args)
{
    LogConfig *lc = (LogConfig *) args;

    /* We haven't freed the logger memory yet */
    if (MemoryInfoGetPointer(i) != lc)
    {
        Log(lc, LOG_WARNING, "%s:%d: %lu bytes of memory at %p leaked.",
            MemoryInfoGetFile(i), MemoryInfoGetLine(i),
            MemoryInfoGetSize(i), MemoryInfoGetPointer(i));

        MemoryHexDump(i, TelodendriaHexDump, lc);
    }
}

void
TelodendriaPrintHeader(LogConfig * lc)
{
    size_t i;

    for (i = 0; i < TELODENDRIA_LOGO_HEIGHT; i++)
    {
        Log(lc, LOG_INFO, "%s", TelodendriaLogo[i]);
    }

    for (i = 0; i < TELODENDRIA_HEADER_HEIGHT; i++)
    {
        Log(lc, LOG_INFO, "%s", TelodendriaHeader[i]);
    }

    Log(lc, LOG_INFO, "Telodendria v" TELODENDRIA_VERSION);
    Log(lc, LOG_INFO, "");
    Log(lc, LOG_INFO,
        "Copyright (C) 2023 Jordan Bancino <@jordan:bancino.net>");
    Log(lc, LOG_INFO,
        "Documentation/Support: https://telodendria.io");
    Log(lc, LOG_INFO, "");
}

