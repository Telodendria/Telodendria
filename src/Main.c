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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <grp.h>
#include <pwd.h>

#include <Telodendria.h>
#include <Memory.h>
#include <TelodendriaConfig.h>
#include <Log.h>
#include <HashMap.h>
#include <Json.h>
#include <HttpServer.h>
#include <Matrix.h>
#include <Db.h>
#include <Cron.h>
#include <Uia.h>

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

int
main(int argc, char **argv)
{
    int exit = EXIT_SUCCESS;

    /* Arg parsing */
    int opt;
    int flags = 0;
    char *configArg = NULL;

    /* Config file */
    Stream *configFile = NULL;
    HashMap *config = NULL;

    /* Program configuration */
    TelodendriaConfig *tConfig = NULL;

    /* User validation */
    struct passwd *userInfo = NULL;
    struct group *groupInfo = NULL;

    /* Signal handling */
    struct sigaction sigAction;

    MatrixHttpHandlerArgs matrixArgs;
    Cron *cron = NULL;

    memset(&matrixArgs, 0, sizeof(matrixArgs));

    if (!LogConfigGlobal())
    {
        printf("Fatal error: unable to allocate memory for logger.\n");
        return EXIT_FAILURE;
    }

    MemoryHook(TelodendriaMemoryHook, NULL);

    TelodendriaPrintHeader();

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

    if (flags & ARG_VERBOSE)
    {
        LogConfigLevelSet(LogConfigGlobal(), LOG_DEBUG);
    }

    if (flags & ARG_VERSION)
    {
        goto finish;
    }

    if (!configArg)
    {
        Log(LOG_ERR, "No configuration file specified.");
        exit = EXIT_FAILURE;
        goto finish;
    }
    else if (strcmp(configArg, "-") == 0)
    {
        configFile = StreamStdin();
    }
    else
    {
        StreamClose(StreamStdin());

        configFile = StreamOpen(configArg, "r");
        if (!configFile)
        {
            Log(LOG_ERR, "Unable to open configuration file '%s' for reading.", configArg);
            exit = EXIT_FAILURE;
            goto finish;
        }
    }

    Log(LOG_NOTICE, "Processing configuration file '%s'.", configArg);

    config = JsonDecode(configFile);
    StreamClose(configFile);

    if (!config)
    {
        Log(LOG_ERR, "Syntax error in configuration file.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    tConfig = TelodendriaConfigParse(config, LogConfigGlobal());
    JsonFree(config);

    if (!tConfig)
    {
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (flags & ARG_CONFIGTEST)
    {
        Log(LOG_INFO, "Configuration is OK.");
        goto finish;
    }

    if (!tConfig->logTimestamp || strcmp(tConfig->logTimestamp, "default") != 0)
    {
        LogConfigTimeStampFormatSet(LogConfigGlobal(), tConfig->logTimestamp);
    }
    else
    {
        Free(tConfig->logTimestamp);
        tConfig->logTimestamp = NULL;
    }

    if (tConfig->flags & TELODENDRIA_LOG_COLOR)
    {
        LogConfigFlagSet(LogConfigGlobal(), LOG_FLAG_COLOR);
    }
    else
    {
        LogConfigFlagClear(LogConfigGlobal(), LOG_FLAG_COLOR);
    }

    LogConfigLevelSet(LogConfigGlobal(), flags & ARG_VERBOSE ? LOG_DEBUG : tConfig->logLevel);

    if (chdir(tConfig->dataDir) != 0)
    {
        Log(LOG_ERR, "Unable to change into data directory: %s.", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(LOG_DEBUG, "Changed working directory to: %s", tConfig->dataDir);
    }


    if (tConfig->flags & TELODENDRIA_LOG_FILE)
    {
        Stream *logFile = StreamOpen("telodendria.log", "a");

        if (!logFile)
        {
            Log(LOG_ERR, "Unable to open log file for appending.");
            exit = EXIT_FAILURE;
            goto finish;
        }

        Log(LOG_INFO, "Logging to the log file. Check there for all future messages.");
        LogConfigOutputSet(LogConfigGlobal(), logFile);
    }
    else if (tConfig->flags & TELODENDRIA_LOG_STDOUT)
    {
        Log(LOG_DEBUG, "Already logging to standard output.");
    }
    else if (tConfig->flags & TELODENDRIA_LOG_SYSLOG)
    {
        Log(LOG_INFO, "Logging to the syslog. Check there for all future messages.");
        LogConfigFlagSet(LogConfigGlobal(), LOG_FLAG_SYSLOG);

        openlog("telodendria", LOG_PID | LOG_NDELAY, LOG_DAEMON);
        /* Always log everything, because the Log API will control what
         * messages get passed to the syslog */
        setlogmask(LOG_UPTO(LOG_DEBUG));
    }
    else
    {
        Log(LOG_ERR, "Unknown logging method in flags: '%d'", tConfig->flags);
        Log(LOG_ERR, "This is a programmer error; please report it.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(LOG_DEBUG, "Configuration:");
    LogConfigIndent(LogConfigGlobal());
    Log(LOG_DEBUG, "Listen On: %d", tConfig->listenPort);
    Log(LOG_DEBUG, "Server Name: %s", tConfig->serverName);
    Log(LOG_DEBUG, "Base URL: %s", tConfig->baseUrl);
    Log(LOG_DEBUG, "Identity Server: %s", tConfig->identityServer);
    Log(LOG_DEBUG, "Run As: %s:%s", tConfig->uid, tConfig->gid);
    Log(LOG_DEBUG, "Data Directory: %s", tConfig->dataDir);
    Log(LOG_DEBUG, "Threads: %d", tConfig->threads);
    Log(LOG_DEBUG, "Max Connections: %d", tConfig->maxConnections);
    Log(LOG_DEBUG, "Max Cache: %ld", tConfig->maxCache);
    Log(LOG_DEBUG, "Flags: %x", tConfig->flags);
    LogConfigUnindent(LogConfigGlobal());

    /* Arguments to pass into the HTTP handler */
    matrixArgs.config = tConfig;

    /* Bind the socket before possibly dropping permissions */
    httpServer = HttpServerCreate(HTTP_FLAG_NONE, tConfig->listenPort, tConfig->threads,
             tConfig->maxConnections, MatrixHttpHandler, &matrixArgs);
    if (!httpServer)
    {
        Log(LOG_ERR, "Unable to create HTTP server on port %d: %s",
            tConfig->listenPort, strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(LOG_DEBUG, "Running as uid:gid: %d:%d.", getuid(), getgid());

    if (tConfig->uid && tConfig->gid)
    {
        userInfo = getpwnam(tConfig->uid);
        groupInfo = getgrnam(tConfig->gid);

        if (!userInfo || !groupInfo)
        {
            Log(LOG_ERR, "Unable to locate the user/group specified in the configuration.");
            exit = EXIT_FAILURE;
            goto finish;
        }
        else
        {
            Log(LOG_DEBUG, "Found user/group information using getpwnam() and getgrnam().");
        }
    }
    else
    {
        Log(LOG_DEBUG, "No user/group info specified in the config.");
    }

    if (getuid() == 0)
    {
        if (userInfo && groupInfo)
        {
            if (setgid(groupInfo->gr_gid) != 0 || setuid(userInfo->pw_uid) != 0)
            {
                Log(LOG_ERR, "Unable to set process uid/gid.");
                exit = EXIT_FAILURE;
                goto finish;
            }
            else
            {
                Log(LOG_DEBUG, "Set uid/gid to %s:%s.", tConfig->uid, tConfig->gid);
            }
        }
        else
        {
            Log(LOG_WARNING, "We are running as root, and we are not dropping to another user");
            Log(LOG_WARNING, "because none was specified in the configuration file.");
            Log(LOG_WARNING, "This is probably a security issue.");
        }
    }
    else
    {
        Log(LOG_WARNING, "Not setting root directory, because we are not root.");

        if (tConfig->uid && tConfig->gid)
        {
            if (getuid() != userInfo->pw_uid || getgid() != groupInfo->gr_gid)
            {
                Log(LOG_WARNING, "Not running as the uid/gid specified in the configuration.");
            }
            else
            {
                Log(LOG_DEBUG, "Running as the uid/gid specified in the configuration.");
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


    if (!tConfig->maxCache)
    {
        Log(LOG_WARNING, "Database caching is disabled.");
        Log(LOG_WARNING, "If this is not what you intended, check the config file");
        Log(LOG_WARNING, "and ensure that maxCache is a valid number of bytes.");
    }

    matrixArgs.db = DbOpen(".", tConfig->maxCache);

    if (!matrixArgs.db)
    {
        Log(LOG_ERR, "Unable to open data directory as a database.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    cron = CronCreate(60 * 1000);  /* 1-minute tick */
    if (!cron)
    {
        Log(LOG_ERR, "Unable to set up job scheduler.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(LOG_DEBUG, "Registering jobs...");

    CronEvery(cron, 30 * 60 * 1000, (JobFunc *) UiaCleanup, &matrixArgs);

    Log(LOG_NOTICE, "Starting job scheduler...");
    CronStart(cron);

    Log(LOG_NOTICE, "Starting server...");

    if (!HttpServerStart(httpServer))
    {
        Log(LOG_ERR, "Unable to start HTTP server.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(LOG_INFO, "Listening on port: %d", tConfig->listenPort);

    sigAction.sa_handler = TelodendriaSignalHandler;
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sigAction, NULL) < 0)
    {
        Log(LOG_ERR, "Unable to install signal handler.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    /* Block this thread until the server is terminated by a signal
     * handler */
    HttpServerJoin(httpServer);

finish:
    Log(LOG_NOTICE, "Shutting down...");
    if (httpServer)
    {
        HttpServerFree(httpServer);
        Log(LOG_DEBUG, "Freed HTTP Server.");
    }

    if (cron)
    {
        CronStop(cron);
        CronFree(cron);
        Log(LOG_DEBUG, "Stopped and freed job scheduler.");
    }

    /*
     * If we're not logging to standard output, then we can close it. Otherwise,
     * if we are logging to stdout, LogConfigFree() will close it for us.
     */
    if (tConfig && !(tConfig->flags & TELODENDRIA_LOG_STDOUT))
    {
        StreamClose(StreamStdout());
    }

    DbClose(matrixArgs.db);

    TelodendriaConfigFree(tConfig);

    Log(LOG_DEBUG, "Exiting with code '%d'.", exit);

    /*
     * Uninstall the memory hook because it uses the Log
     * API, whose configuration is being freed now, so it
     * won't work anymore.
     */
    MemoryHook(NULL, NULL);

    LogConfigFree(LogConfigGlobal());

    /* Standard error should never have been opened, but just in case
     * it was, this doesn't hurt anything. */
    StreamClose(StreamStderr());

    /* Generate a memory report if any leaks occurred. At this point no
     * memory should be allocated. */
    TelodendriaGenerateMemReport();

    MemoryFreeAll();
    return exit;
}
