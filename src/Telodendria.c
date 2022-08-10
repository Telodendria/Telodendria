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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <TelodendriaConfig.h>

#include <Log.h>
#include <HashMap.h>
#include <Config.h>

typedef enum ArgFlag
{
    ARG_VERSION = (1 << 0),
    ARG_USAGE = (1 << 1)
} ArgFlag;

static void
TelodendriaPrintHeader(LogConfig * lc)
{
    Log(lc, LOG_MESSAGE,
        " _____    _           _                _      _");
    Log(lc, LOG_MESSAGE,
        "|_   _|__| | ___   __| | ___ _ __   __| |_ __(_) __ _");
    Log(lc, LOG_MESSAGE,
        "  | |/ _ \\ |/ _ \\ / _` |/ _ \\ '_ \\ / _` | '__| |/ _` |");
    Log(lc, LOG_MESSAGE,
        "  | |  __/ | (_) | (_| |  __/ | | | (_| | |  | | (_| |");
    Log(lc, LOG_MESSAGE,
      "  |_|\\___|_|\\___/ \\__,_|\\___|_| |_|\\__,_|_|  |_|\\__,_|");
    Log(lc, LOG_MESSAGE, "Telodendria v" TELODENDRIA_VERSION);
    Log(lc, LOG_MESSAGE, "");
    Log(lc, LOG_MESSAGE,
        "Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>");
    Log(lc, LOG_MESSAGE,
        "Documentation/Support: https://telodendria.io");
    Log(lc, LOG_MESSAGE, "");
}

static void
TelodendriaPrintUsage(LogConfig * lc)
{
    Log(lc, LOG_MESSAGE, "Usage:");
    Log(lc, LOG_MESSAGE, "  -c <file>    Configuration file ('-' for stdin).");
    Log(lc, LOG_MESSAGE, "  -V           Print the header, then exit.");
    Log(lc, LOG_MESSAGE, "  -h           Print this usage, then exit.");
}

int
main(int argc, char **argv)
{
    LogConfig *lc;
    int exit = EXIT_SUCCESS;

    /* Arg parsing */
    int opt;
    int flags = 0;
    char *configArg = NULL;

    /* Config file */
    FILE *configFile = NULL;
    ConfigParseResult *configParseResult = NULL;
    HashMap *config = NULL;

    /* Program configuration */
    TelodendriaConfig *tConfig = NULL;

    lc = LogConfigCreate();

    if (!lc)
    {
        printf("Fatal error: unable to allocate memory for logger.\n");
        return EXIT_FAILURE;
    }

    TelodendriaPrintHeader(lc);

    while ((opt = getopt(argc, argv, "c:Vh")) != -1)
    {
        switch (opt)
        {
            case 'c':
                configArg = optarg;
                break;
            case 'V':
                flags |= ARG_VERSION;
                break;
            case 'h':
                flags |= ARG_USAGE;
            default:
                break;
        }
    }

    if (flags & ARG_VERSION)
    {
        goto finish;
    }

    if (flags & ARG_USAGE)
    {
        TelodendriaPrintUsage(lc);
        goto finish;
    }

    if (!configArg)
    {
        Log(lc, LOG_ERROR, "No configuration file specified.");
        TelodendriaPrintUsage(lc);
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (strcmp(configArg, "-") == 0)
    {
        configFile = stdout;
    }
    else
    {
        configFile = fopen(configArg, "r");
        if (!configFile)
        {
            Log(lc, LOG_ERROR, "Unable to open configuration file '%s' for reading.", configArg);
            exit = EXIT_FAILURE;
            goto finish;
        }
    }

    Log(lc, LOG_TASK, "Processing configuration file '%s'.", configArg);

    configParseResult = ConfigParse(configFile);
    if (!ConfigParseResultOk(configParseResult))
    {
        Log(lc, LOG_ERROR, "Syntax error on line %d.",
            ConfigParseResultLineNumber(configParseResult));
        exit = EXIT_FAILURE;
        goto finish;
    }

    config = ConfigParseResultGet(configParseResult);
    ConfigParseResultFree(configParseResult);

    fclose(configFile);

    tConfig = TelodendriaConfigParse(config, lc);
    if (!tConfig)
    {
        exit = EXIT_FAILURE;
        goto finish;
    }

    ConfigFree(config);

    Log(lc, LOG_DEBUG, "Configuration:");
    LogConfigIndent(lc);
    Log(lc, LOG_DEBUG, "Listen On: %s:%s", tConfig->listenHost, tConfig->listenPort);
    Log(lc, LOG_DEBUG, "Server Name: %s", tConfig->serverName);
    Log(lc, LOG_DEBUG, "Chroot: %s", tConfig->chroot);
    Log(lc, LOG_DEBUG, "Run As: %s:%s", tConfig->uid, tConfig->gid);
    Log(lc, LOG_DEBUG, "Data Directory: %s", tConfig->dataDir);
    Log(lc, LOG_DEBUG, "Threads: %d", tConfig->threads);
    Log(lc, LOG_DEBUG, "Flags: %x", tConfig->flags);
    LogConfigUnindent(lc);

finish:
    Log(lc, LOG_DEBUG, "Exiting with code '%d'.", exit);
    TelodendriaConfigFree(tConfig);
    return exit;
}
