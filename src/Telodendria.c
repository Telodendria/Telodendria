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

#include <Memory.h>
#include <TelodendriaConfig.h>
#include <Log.h>
#include <HashMap.h>
#include <Config.h>
#include <HttpServer.h>
#include <Matrix.h>
#include <Db.h>

static void
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

    Log(lc, LOG_DEBUG, "%s:%d: %s %lu bytes of memory at %p.",
        MemoryInfoGetFile(i), MemoryInfoGetLine(i),
        action, MemoryInfoGetSize(i),
        MemoryInfoGetPointer(i));
}

static void
TelodendriaMemoryIterator(MemoryInfo * i, void *args)
{
    LogConfig *lc = (LogConfig *) args;

    /* We haven't freed the logger memory yet */
    if (MemoryInfoGetPointer(i) != lc)
    {
        Log(lc, LOG_WARNING, "%s:%d: %lu bytes of memory at %p leaked.",
            MemoryInfoGetFile(i), MemoryInfoGetLine(i),
            MemoryInfoGetSize(i), MemoryInfoGetPointer(i));
    }
}

static HttpServer *httpServer = NULL;

static void
TelodendriaSignalHandler(int signalNo)
{
    (void) signalNo;
    HttpServerStop(httpServer);
}

typedef enum ArgFlag
{
    ARG_VERSION = (1 << 0),
    ARG_CONFIGTEST = (1 << 1),
    ARG_VERBOSE = (1 << 2)
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

int
main(int argc, char **argv)
{
    LogConfig *lc;
    int exit = EXIT_SUCCESS;

    /* Arg parsing */
    int opt;
    int flags = 0;
    char *configArg = "/etc/telodendria.conf";

    /* Config file */
    FILE *configFile = NULL;
    ConfigParseResult *configParseResult = NULL;
    HashMap *config = NULL;

    /* Program configuration */
    TelodendriaConfig *tConfig = NULL;

    /* User validation */
    struct passwd *userInfo = NULL;
    struct group *groupInfo = NULL;

    /* Signal handling */
    struct sigaction sigAction;

    MatrixHttpHandlerArgs matrixArgs;

    memset(&matrixArgs, 0, sizeof(matrixArgs));

    lc = LogConfigCreate();

    if (!lc)
    {
        printf("Fatal error: unable to allocate memory for logger.\n");
        return EXIT_FAILURE;
    }

    MemoryHook(TelodendriaMemoryHook, lc);

    TelodendriaPrintHeader(lc);

#ifdef __OpenBSD__
    Log(lc, LOG_DEBUG, "Attempting pledge...");

    if (pledge("stdio rpath wpath cpath flock inet dns getpw id unveil", NULL) != 0)
    {
        Log(lc, LOG_ERROR, "Pledge failed: %s", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
#endif

    while ((opt = getopt(argc, argv, "f:Vvn")) != -1)
    {
        switch (opt)
        {
            case 'f':
                configArg = optarg;
                break;
            case 'V':
                flags |= ARG_VERSION;
                break;
            case 'v':
                flags |= ARG_VERBOSE;
                break;
            case 'n':
                flags |= ARG_CONFIGTEST;
                break;
            case '?':
                exit = EXIT_FAILURE;
                goto finish;
            default:
                break;
        }
    }

    if (flags & ARG_VERSION)
    {
        goto finish;
    }

    if (strcmp(configArg, "-") == 0)
    {
        configFile = stdin;
    }
    else
    {
        fclose(stdin);
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

    if (flags & ARG_CONFIGTEST)
    {
        Log(lc, LOG_MESSAGE, "Configuration is OK.");
        goto finish;
    }

#ifdef __OpenBSD__
    if (unveil(tConfig->dataDir, "rwc") != 0)
    {
        Log(lc, LOG_ERROR, "Unveil of data directory failed: %s", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }

    unveil(NULL, NULL);            /* Done with unveil(), so disable it */
#endif

    LogConfigTimeStampFormatSet(lc, tConfig->logTimestamp);

    if (tConfig->flags & TELODENDRIA_LOG_COLOR)
    {
        LogConfigFlagSet(lc, LOG_FLAG_COLOR);
    }
    else
    {
        LogConfigFlagClear(lc, LOG_FLAG_COLOR);
    }

    LogConfigLevelSet(lc, flags & ARG_VERBOSE ? LOG_DEBUG : tConfig->logLevel);

    if (chdir(tConfig->dataDir) != 0)
    {
        Log(lc, LOG_ERROR, "Unable to change into data directory: %s.", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(lc, LOG_DEBUG, "Changed working directory to: %s", tConfig->dataDir);
    }


    if (tConfig->flags & TELODENDRIA_LOG_FILE)
    {
        FILE *logFile = fopen("telodendria.log", "a");

        if (!logFile)
        {
            Log(lc, LOG_ERROR, "Unable to open log file for appending.");
            exit = EXIT_FAILURE;
            goto finish;
        }

        Log(lc, LOG_MESSAGE, "Logging to the log file. Check there for all future messages.");
        LogConfigOutputSet(lc, logFile);
    }
    else if (tConfig->flags & TELODENDRIA_LOG_STDOUT)
    {
        Log(lc, LOG_DEBUG, "Already logging to standard output.");
    }
    else if (tConfig->flags & TELODENDRIA_LOG_SYSLOG)
    {
        Log(lc, LOG_MESSAGE, "Logging to the syslog. Check there for all future messages.");
        LogConfigFlagSet(lc, LOG_FLAG_SYSLOG);

        openlog("telodendria", LOG_PID | LOG_NDELAY, LOG_DAEMON);
        /* Always log everything, because the Log API will control what
         * messages get passed to the syslog */
        setlogmask(LOG_UPTO(LOG_DEBUG));
    }
    else
    {
        Log(lc, LOG_ERROR, "Unknown logging method in flags: '%d'", tConfig->flags);
        Log(lc, LOG_ERROR, "This is a programmer error; please report it.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(lc, LOG_DEBUG, "Configuration:");
    LogConfigIndent(lc);
    Log(lc, LOG_DEBUG, "Listen On: %d", tConfig->listenPort);
    Log(lc, LOG_DEBUG, "Server Name: %s", tConfig->serverName);
    Log(lc, LOG_DEBUG, "Base URL: %s", tConfig->baseUrl);
    Log(lc, LOG_DEBUG, "Identity Server: %s", tConfig->identityServer);
    Log(lc, LOG_DEBUG, "Run As: %s:%s", tConfig->uid, tConfig->gid);
    Log(lc, LOG_DEBUG, "Data Directory: %s", tConfig->dataDir);
    Log(lc, LOG_DEBUG, "Threads: %d", tConfig->threads);
    Log(lc, LOG_DEBUG, "Max Connections: %d", tConfig->maxConnections);
    Log(lc, LOG_DEBUG, "Max Cache: %ld", tConfig->maxCache);
    Log(lc, LOG_DEBUG, "Flags: %x", tConfig->flags);
    LogConfigUnindent(lc);

    /* Arguments to pass into the HTTP handler */
    matrixArgs.lc = lc;
    matrixArgs.config = tConfig;

    /* Bind the socket before possibly dropping permissions */
    httpServer = HttpServerCreate(tConfig->listenPort, tConfig->threads, tConfig->maxConnections,
                                  MatrixHttpHandler, &matrixArgs);
    if (!httpServer)
    {
        Log(lc, LOG_ERROR, "Unable to create HTTP server on port %d: %s",
            tConfig->listenPort, strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(lc, LOG_DEBUG, "Running as uid:gid: %d:%d.", getuid(), getgid());

    if (tConfig->uid && tConfig->gid)
    {
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
    }
    else
    {
        Log(lc, LOG_DEBUG, "No user/group info specified in the config.");
    }

    if (getuid() == 0)
    {
#ifndef __OpenBSD__                /* chroot() is only useful without
                                    * unveil() */
        if (chroot(".") == 0)
        {
            Log(lc, LOG_DEBUG, "Changed the root directory to: %s.", tConfig->dataDir);
        }
        else
        {
            Log(lc, LOG_WARNING, "Unable to chroot into directory: %s.", tConfig->dataDir);
        }
#endif

        if (userInfo && groupInfo)
        {
            if (setgid(groupInfo->gr_gid) != 0 || setuid(userInfo->pw_uid) != 0)
            {
                Log(lc, LOG_ERROR, "Unable to set process uid/gid.");
                exit = EXIT_FAILURE;
                goto finish;
            }
            else
            {
                Log(lc, LOG_DEBUG, "Set uid/gid to %s:%s.", tConfig->uid, tConfig->gid);
            }
        }
        else
        {
            Log(lc, LOG_WARNING, "We are running as root, and we are not dropping to another user");
            Log(lc, LOG_WARNING, "because none was specified in the configuration file.");
            Log(lc, LOG_WARNING, "This is probably a security issue.");
        }
    }
    else
    {
        Log(lc, LOG_WARNING, "Not setting root directory, because we are not root.");

        if (tConfig->uid && tConfig->gid)
        {
            if (getuid() != userInfo->pw_uid || getgid() != groupInfo->gr_gid)
            {
                Log(lc, LOG_WARNING, "Not running as the uid/gid specified in the configuration.");
            }
            else
            {
                Log(lc, LOG_DEBUG, "Running as the uid/gid specified in the configuration.");
            }
        }
    }

    /* These config values are no longer needed; don't hold them in
     * memory anymore */
    Free(tConfig->dataDir);
    Free(tConfig->uid);
    Free(tConfig->gid);

    tConfig->dataDir = NULL;
    tConfig->uid = NULL;
    tConfig->gid = NULL;

    matrixArgs.db = DbOpen(".", tConfig->maxCache);

    if (!tConfig->maxCache)
    {
        Log(lc, LOG_WARNING, "Max-cache is set to zero; caching is disabled.");
    }

    if (!matrixArgs.db)
    {
        Log(lc, LOG_ERROR, "Unable to open data directory as a database.");
        exit = EXIT_FAILURE;
        goto finish;
    }

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
        Log(lc, LOG_DEBUG, "Freed HTTP Server.");
    }

    /*
     * If we're not logging to standard output, then we can close it. Otherwise,
     * if we are logging to stdout, LogConfigFree() will close it for us.
     */
    if (!tConfig || !(tConfig->flags & TELODENDRIA_LOG_STDOUT))
    {
        fclose(stdout);
    }

    TelodendriaConfigFree(tConfig);
    DbClose(matrixArgs.db);

    Log(lc, LOG_DEBUG, "");
    MemoryIterate(TelodendriaMemoryIterator, lc);
    Log(lc, LOG_DEBUG, "");

    Log(lc, LOG_DEBUG, "Exiting with code '%d'.", exit);
    LogConfigFree(lc);

    MemoryFreeAll();

    fclose(stderr);
    return exit;
}
