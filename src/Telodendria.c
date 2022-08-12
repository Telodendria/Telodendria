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
#include <errno.h>
#include <signal.h>

#include <grp.h>
#include <pwd.h>

#include <TelodendriaConfig.h>
#include <Log.h>
#include <HashMap.h>
#include <Config.h>
#include <HttpServer.h>

HttpServer *httpServer = NULL;

static void
TelodendriaHttpHandler(HttpRequest * req, HttpResponse * res, void *args)
{

}

static void
TelodendriaSignalHandler(int signalNo)
{
    (void) signalNo;
    HttpServerStop(httpServer);
}

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

    /* User validation */
    struct passwd *userInfo;
    struct group *groupInfo;

    /* Signal handling */
    struct sigaction sigAction;

    lc = LogConfigCreate();

    if (!lc)
    {
        printf("Fatal error: unable to allocate memory for logger.\n");
        return EXIT_FAILURE;
    }

    TelodendriaPrintHeader(lc);

#ifdef __OpenBSD__
    Log(lc, LOG_DEBUG, "Attempting pledge...");

    if (pledge("stdio rpath wpath cpath inet dns getpw id unveil", NULL) != 0)
    {
        Log(lc, LOG_ERROR, "Pledge failed: %s", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
#endif

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
        configFile = stdin;
    }
    else
    {
#ifdef __OpenBSD__
        if (unveil(configArg, "r") != 0)
        {
            Log(lc, LOG_ERROR, "Unable to unveil() configuration file '%s' for reading.", configArg);
            exit = EXIT_FAILURE;
            goto finish;
        }
#endif
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

#ifdef __OpenBSD__
    if (unveil(tConfig->chroot, "rwc") != 0)
    {
        Log(lc, LOG_ERROR, "Unveil of data directory failed: %s", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }

    unveil(NULL, NULL);            /* Done with unveil(), so disable it */
#endif

    LogConfigTimeStampFormatSet(lc, tConfig->logTimestamp);

    /* Color is enabled by default in the logger. */
    if (!(tConfig->flags & TELODENDRIA_LOG_COLOR))
    {
        LogConfigFlagClear(lc, LOG_FLAG_COLOR);
    }

    LogConfigLevelSet(lc, tConfig->logLevel);

    if (tConfig->logOut)
    {
        FILE *logFile = fopen(tConfig->logOut, "w");

        if (!logFile)
        {
            Log(lc, LOG_ERROR, "Unable to open log file '%s' for writing.", tConfig->logOut);
            exit = EXIT_FAILURE;
            goto finish;
        }

        Log(lc, LOG_DEBUG, "Redirecting future output to '%s'.", tConfig->logOut);
        LogConfigOutputSet(lc, logFile);
    }

    Log(lc, LOG_DEBUG, "Configuration:");
    LogConfigIndent(lc);
    Log(lc, LOG_DEBUG, "Listen On: %d", tConfig->listenPort);
    Log(lc, LOG_DEBUG, "Server Name: %s", tConfig->serverName);
    Log(lc, LOG_DEBUG, "Chroot: %s", tConfig->chroot);
    Log(lc, LOG_DEBUG, "Run As: %s:%s", tConfig->uid, tConfig->gid);
    Log(lc, LOG_DEBUG, "Data Directory: %s", tConfig->dataDir);
    Log(lc, LOG_DEBUG, "Threads: %d", tConfig->threads);
    Log(lc, LOG_DEBUG, "Flags: %x", tConfig->flags);
    LogConfigUnindent(lc);

    if (chdir(tConfig->chroot) != 0)
    {
        Log(lc, LOG_ERROR, "Unable to change into data directory: %s.", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(lc, LOG_DEBUG, "Changed working directory to: %s", tConfig->chroot);
    }

    Log(lc, LOG_DEBUG, "Running as uid:gid: %d:%d.", getuid(), getgid());

    userInfo = getpwnam(tConfig->uid);
    groupInfo = getgrnam(tConfig->gid);

    if (!userInfo || !groupInfo)
    {
        Log(lc, LOG_ERROR, "Unable to locate the user/group specified in the configuration.");
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(lc, LOG_DEBUG, "Found user/group information using getpwnam() and getgrnam().");
    }

    /* Bind the socket before possibly dropping permissions */
    httpServer = HttpServerCreate(tConfig->listenPort, tConfig->threads,
                                  TelodendriaHttpHandler, NULL);
    if (!httpServer)
    {
        Log(lc, LOG_ERROR, "Unable to create HTTP server on port %d: %s",
            tConfig->listenPort, strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (getuid() == 0)
    {
#ifndef __OpenBSD__
        if (chroot(tConfig->chroot) == 0)
        {
            Log(lc, LOG_DEBUG, "Changed the root directory to: %s.", tConfig->chroot);
        }
        else
        {
            Log(lc, LOG_WARNING, "Unable to chroot into directory: %s.", tConfig->chroot);
        }
#else
        Log(lc, LOG_DEBUG, "Not attempting chroot() after pledge() and unveil().");
#endif

        if (setgid(groupInfo->gr_gid) != 0 || setuid(userInfo->pw_uid) != 0)
        {
            Log(lc, LOG_WARNING, "Unable to set process uid/gid.");
        }
        else
        {
            Log(lc, LOG_DEBUG, "Set uid/gid to %s:%s.", tConfig->uid, tConfig->gid);
        }
    }
    else
    {
        Log(lc, LOG_DEBUG, "Not changing root directory, because we are not root.");

        if (getuid() != userInfo->pw_uid || getgid() != groupInfo->gr_gid)
        {
            Log(lc, LOG_WARNING, "Not running as the uid/gid specified in the configuration.");
        }
        else
        {
            Log(lc, LOG_DEBUG, "Running as the uid/gid specified in the configuration.");
        }
    }

    /* These config values are no longer needed; don't hold them in
     * memory anymore */
    free(tConfig->chroot);
    free(tConfig->uid);
    free(tConfig->gid);

    tConfig->chroot = NULL;
    tConfig->uid = NULL;
    tConfig->gid = NULL;

    Log(lc, LOG_TASK, "Starting server...");

    if (!HttpServerStart(httpServer))
    {
        Log(lc, LOG_ERROR, "Unable to start HTTP server.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(lc, LOG_MESSAGE, "Listening on port: %d", tConfig->listenPort);

    sigAction.sa_handler = TelodendriaSignalHandler;
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sigAction, NULL) < 0)
    {
        Log(lc, LOG_ERROR, "Unable to install signal handler.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    /* Block this thread until the server is terminated by a signal
     * handler */
    HttpServerJoin(httpServer);

finish:
    Log(lc, LOG_TASK, "Shutting down...");
    if (httpServer)
    {
        HttpServerFree(httpServer);
    }
    Log(lc, LOG_DEBUG, "Exiting with code '%d'.", exit);
    TelodendriaConfigFree(tConfig);
    LogConfigFree(lc);
    return exit;
}
