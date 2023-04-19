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

#include <HttpRouter.h>
#include <Routes.h>

#include <time.h>

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
TelodendriaMemoryHook(MemoryAction a, MemoryInfo * i, void *args)
{
    char *action;

    (void) args;

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

    Log(a == MEMORY_BAD_POINTER ? LOG_WARNING : LOG_DEBUG,
        "%s:%d: %s %lu bytes of memory at %p.",
        MemoryInfoGetFile(i), MemoryInfoGetLine(i),
        action, MemoryInfoGetSize(i),
        MemoryInfoGetPointer(i));
}

static void
HexDump(size_t off, char *hexBuf, char *asciiBuf, void *args)
{
    FILE *report = args;

    if (hexBuf && asciiBuf)
    {
        fprintf(report, "%04lx: %s | %s |\n", off, hexBuf, asciiBuf);
    }
    else
    {
        fprintf(report, "%04lx\n", off);
    }
}


static void
MemoryIterator(MemoryInfo * i, void *args)
{
    FILE *report = args;

    fprintf(report, "%s:%d: %lu bytes at %p\n",
            MemoryInfoGetFile(i), MemoryInfoGetLine(i),
            MemoryInfoGetSize(i), MemoryInfoGetPointer(i));

    MemoryHexDump(i, HexDump, report);

    fprintf(report, "\n");
}

void
TelodendriaGenerateMemReport(void)
{
    static const char *reportName = "Memory.txt";

    /*
     * Use C standard IO instead of the Stream or Io APIs, because
     * those use the Memory API, and that's exactly what we're trying
     * to assess, so using it would generate false positives. None of
     * this code should leak memory.
     */
    FILE *report;
    time_t currentTime;
    struct tm *timeInfo;
    char tsBuffer[1024];

    if (!MemoryAllocated())
    {
        /* No memory leaked, no need to write the report. This is the
         * ideal situation; we only want the report to show up if leaks
         * occurred. */
        return;
    }

    report = fopen(reportName, "a");
    if (!report)
    {
        return;
    }

    currentTime = time(NULL);
    timeInfo = localtime(&currentTime);
    strftime(tsBuffer, sizeof(tsBuffer), "%c", timeInfo);

    fprintf(report, "---------- Telodendria Memory Report ----------\n");
    fprintf(report, "Date: %s\n", tsBuffer);
    fprintf(report, "Total Bytes: %lu\n", MemoryAllocated());
    fprintf(report, "\n");

    MemoryIterate(MemoryIterator, report);

    fclose(report);
}

void
TelodendriaPrintHeader(void)
{
    size_t i;

    for (i = 0; i < TELODENDRIA_LOGO_HEIGHT; i++)
    {
        Log(LOG_INFO, "%s", TelodendriaLogo[i]);
    }

    for (i = 0; i < TELODENDRIA_HEADER_HEIGHT; i++)
    {
        Log(LOG_INFO, "%s", TelodendriaHeader[i]);
    }

    Log(LOG_INFO, "Telodendria v" TELODENDRIA_VERSION);
    Log(LOG_INFO, "");
    Log(LOG_INFO,
        "Copyright (C) 2023 Jordan Bancino <@jordan:bancino.net>");
    Log(LOG_INFO,
        "Documentation/Support: https://telodendria.io");
    Log(LOG_INFO, "");
}
