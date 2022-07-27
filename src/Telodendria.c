#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

    lc = LogConfigCreate();

    /* TODO: Remove */
    LogConfigLevelSet(lc, LOG_DEBUG);

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

    Log(lc, LOG_MESSAGE, "Using configuration file '%s'.", configArg);

    Log(lc, LOG_DEBUG, "Executing ConfigParse()");

    /* Read config here */
    configParseResult = ConfigParse(configFile);

    Log(lc, LOG_DEBUG, "Exitting ConfigParse()");

    if (!ConfigParseResultOk(configParseResult))
    {
        Log(lc, LOG_ERROR, "Syntax error on line %d.",
            ConfigParseResultLineNumber(configParseResult));
        exit = EXIT_FAILURE;
        goto finish;
    }

    config = ConfigParseResultGet(configParseResult);
    ConfigParseResultFree(configParseResult);

    Log(lc, LOG_DEBUG, "Closing configuration file.");
    fclose(configFile);

    /* Configure log file */

finish:
    if (config)
    {
        Log(lc, LOG_DEBUG, "Freeing configuration structure.");
        ConfigFree(config);
    }
    Log(lc, LOG_DEBUG, "Freeing log configuration and exiting with code '%d'.", exit);
    LogConfigFree(lc);
    return exit;
}
