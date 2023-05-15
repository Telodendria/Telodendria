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
#include <Runtime.h>
#include <Array.h>
#include <Stream.h>
#include <Log.h>
#include <Memory.h>

#include <stdlib.h>
#include <time.h>
#include <libgen.h>

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
GenerateMemoryReport(int argc, char **argv)
{
    char reportName[128];
    char *namePtr;

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
    size_t i;

    if (!MemoryAllocated())
    {
        /* No memory leaked, no need to write the report. This is the
         * ideal situation; we only want the report to show up if leaks
         * occurred. */
        return;
    }

    snprintf(reportName, sizeof(reportName), "%s-leaked.txt", argv[0]);
    /* Make sure the report goes in the current working directory. */
    namePtr = basename(reportName);
    report = fopen(namePtr, "a");
    if (!report)
    {
        return;
    }

    currentTime = time(NULL);
    timeInfo = localtime(&currentTime);
    strftime(tsBuffer, sizeof(tsBuffer), "%c", timeInfo);

    fprintf(report, "---------- Memory Report ----------\n");
    fprintf(report, "Program:");
    for (i = 0; i < argc; i++)
    {
        fprintf(report, " '%s'", argv[i]);
    }
    fprintf(report, "\nDate: %s\n", tsBuffer);
    fprintf(report, "Total Bytes: %lu\n", MemoryAllocated());
    fprintf(report, "\n");

    MemoryIterate(MemoryIterator, report);

    fclose(report);
}
