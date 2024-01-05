/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
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
#include <limits.h>

#include <grp.h>
#include <pwd.h>

#include <Cytoplasm/Args.h>
#include <Cytoplasm/Memory.h>
#include <Config.h>
#include <Cytoplasm/Log.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/HttpServer.h>
#include <Cytoplasm/Db.h>
#include <Cytoplasm/Cron.h>
#include <Uia.h>
#include <Cytoplasm/Util.h>
#include <Cytoplasm/Str.h>

#include <Telodendria.h>
#include <Matrix.h>
#include <User.h>
#include <RegToken.h>
#include <Routes.h>

static Array *httpServers;
static volatile int restart;

static void
SignalHandler(int signal)
{
    size_t i;

    switch (signal)
    {
        case SIGPIPE:
            return;
        case SIGUSR1:
            restart = 1;
            /* Fall through */
        case SIGTERM:
        case SIGINT:
            if (!httpServers)
            {
                return;
            }

            for (i = 0; i < ArraySize(httpServers); i++)
            {
                HttpServer *server = ArrayGet(httpServers, i);

                HttpServerStop(server);
            }
            break;
    }
}

typedef enum ArgFlag
{
    ARG_VERSION = (1 << 0),
    ARG_VERBOSE = (1 << 2)
} ArgFlag;

int
Main(Array * args)
{
    int exit;

    /* Arg parsing */
    ArgParseState arg;
    int opt;
    int flags;
    char *dbPath;

    /* Program configuration */
    Config *tConfig;
    Stream *logFile;

    /* User validation */
    struct passwd *userInfo;
    struct group *groupInfo;

    /* HTTP server management */
    size_t i;
    HttpServer *server;

    /* Signal handling */
    struct sigaction sigAction;

    MatrixHttpHandlerArgs matrixArgs;
    Cron *cron;

    char startDir[PATH_MAX];

    char *token;

start:
    /* Global variables */
    httpServers = NULL;
    restart = 0;

    /* Local variables */
    exit = EXIT_SUCCESS;
    flags = 0;
    dbPath = NULL;
    tConfig = NULL;
    logFile = NULL;
    userInfo = NULL;
    groupInfo = NULL;
    cron = NULL;

    token = NULL;

    memset(&matrixArgs, 0, sizeof(matrixArgs));

    if (!LogConfigGlobal())
    {
        printf("Fatal error: unable to allocate memory for logger.\n");
        return EXIT_FAILURE;
    }

    TelodendriaPrintHeader();

    ArgParseStateInit(&arg);
    while ((opt = ArgParse(&arg, args, "d:Vv")) != -1)
    {
        switch (opt)
        {
            case 'd':
                dbPath = arg.optArg;
                break;
            case 'V':
                flags |= ARG_VERSION;
                break;
            case 'v':
                flags |= ARG_VERBOSE;
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
        MemoryHook(TelodendriaMemoryHook, (void *) ARG_VERBOSE);
    }
    else
    {
        MemoryHook(TelodendriaMemoryHook, NULL);
    }

    if (flags & ARG_VERSION)
    {
        goto finish;
    }

    if (!dbPath)
    {
        Log(LOG_ERR, "No database directory specified.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (!getcwd(startDir, PATH_MAX))
    {
        Log(LOG_ERR, "Unable to determine current working directory.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (chdir(dbPath) != 0)
    {
        Log(LOG_ERR, "Unable to change into data directory: %s.", strerror(errno));
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(LOG_DEBUG, "Changed working directory to: %s", dbPath);
    }

    matrixArgs.db = DbOpen(".", 0);
    if (!matrixArgs.db)
    {
        Log(LOG_ERR, "Unable to open data directory as a database.");
        exit = EXIT_FAILURE;
        goto finish;
    }
    else
    {
        Log(LOG_DEBUG, "Opened database.");
    }

    if (!ConfigExists(matrixArgs.db))
    {
        RegTokenInfo *info;

        Log(LOG_NOTICE, "No configuration exists in the opened database.");
        Log(LOG_NOTICE, "A default configuration will be created, and a");
        Log(LOG_NOTICE, "new single-use registration token that grants all");
        Log(LOG_NOTICE, "privileges will be created so an admin user can");
        Log(LOG_NOTICE, "be created to configure this database using the");
        Log(LOG_NOTICE, "administrator API.");

        if (!ConfigCreateDefault(matrixArgs.db))
        {
            Log(LOG_ERR, "Unable to create default configuration.");
            exit = EXIT_FAILURE;
            goto finish;
        }

        token = StrRandom(32);
        info = RegTokenCreate(matrixArgs.db, token, NULL, UInt64Create(0, 0), Int64Create(0, 1), USER_ALL);
        if (!info)
        {
            Free(token);
            Log(LOG_ERR, "Unable to create admin registration token.");
            exit = EXIT_FAILURE;
            goto finish;
        }

        RegTokenClose(info);
        RegTokenFree(info);

        /* Don't free token, because we need to print it when logging
         * is set up. */
    }

    Log(LOG_NOTICE, "Loading configuration...");

    tConfig = ConfigLock(matrixArgs.db);
    if (!tConfig)
    {
        Log(LOG_ERR, "Error locking the configuration.");
        Log(LOG_ERR, "The configuration object is corrupted or otherwise invalid.");
        Log(LOG_ERR, "Please restore from a backup.");
        exit = EXIT_FAILURE;
        goto finish;
    }
    else if (!tConfig->ok)
    {
        Log(LOG_ERR, tConfig->err);
        exit = EXIT_FAILURE;
        goto finish;
    }

    if (!tConfig->logTimestamp || !StrEquals(tConfig->logTimestamp, "default"))
    {
        LogConfigTimeStampFormatSet(LogConfigGlobal(), tConfig->logTimestamp);
    }
    else
    {
        Free(tConfig->logTimestamp);
        tConfig->logTimestamp = NULL;
    }

    if (tConfig->flags & CONFIG_LOG_COLOR)
    {
        LogConfigFlagSet(LogConfigGlobal(), LOG_FLAG_COLOR);
    }
    else
    {
        LogConfigFlagClear(LogConfigGlobal(), LOG_FLAG_COLOR);
    }

    LogConfigLevelSet(LogConfigGlobal(), flags & ARG_VERBOSE ? LOG_DEBUG : tConfig->logLevel);

    if (tConfig->flags & CONFIG_LOG_FILE)
    {
        logFile = StreamOpen("telodendria.log", "a");

        if (!logFile)
        {
            Log(LOG_ERR, "Unable to open log file for appending.");
            exit = EXIT_FAILURE;
            tConfig->flags &= CONFIG_LOG_STDOUT;
            goto finish;
        }

        Log(LOG_INFO, "Logging to the log file. Check there for all future messages.");
        LogConfigOutputSet(LogConfigGlobal(), logFile);
    }
    else if (tConfig->flags & CONFIG_LOG_STDOUT)
    {
        Log(LOG_DEBUG, "Already logging to standard output.");
    }
    else if (tConfig->flags & CONFIG_LOG_SYSLOG)
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

    /* If a token was created with a default config, print it to the
     * log */
    if (token)
    {
        Log(LOG_NOTICE, "Admin Registration token: %s", token);
        Free(token);
    }

    Log(LOG_DEBUG, "Configuration:");
    LogConfigIndent(LogConfigGlobal());
    Log(LOG_DEBUG, "Server Name: %s", tConfig->serverName);
    Log(LOG_DEBUG, "Base URL: %s", tConfig->baseUrl);
    Log(LOG_DEBUG, "Identity Server: %s", tConfig->identityServer);
    Log(LOG_DEBUG, "Run As: %s:%s", tConfig->uid, tConfig->gid);
    Log(LOG_DEBUG, "Max Cache: %ld", tConfig->maxCache);
    Log(LOG_DEBUG, "Flags: %x", tConfig->flags);
    LogConfigUnindent(LogConfigGlobal());

    httpServers = ArrayCreate();
    if (!httpServers)
    {
        Log(LOG_ERR, "Error setting up HTTP server.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    /* Bind servers before possibly dropping permissions. */
    for (i = 0; i < ArraySize(tConfig->servers); i++)
    {
        HttpServerConfig *serverCfg = ArrayGet(tConfig->servers, i);

        Log(LOG_DEBUG, "HTTP listener: %lu", i);
        LogConfigIndent(LogConfigGlobal());
        Log(LOG_DEBUG, "Port: %hu", serverCfg->port);
        Log(LOG_DEBUG, "Threads: %u", serverCfg->threads);
        Log(LOG_DEBUG, "Max Connections: %u", serverCfg->maxConnections);
        Log(LOG_DEBUG, "Flags: %d", serverCfg->flags);
        Log(LOG_DEBUG, "TLS Cert: %s", serverCfg->tlsCert);
        Log(LOG_DEBUG, "TLS Key: %s", serverCfg->tlsKey);
        LogConfigUnindent(LogConfigGlobal());

        serverCfg->handler = MatrixHttpHandler;
        serverCfg->handlerArgs = &matrixArgs;

        if (serverCfg->flags & HTTP_FLAG_TLS)
        {
            if (UInt64Eq(UtilLastModified(serverCfg->tlsCert), UInt64Create(0, 0)))
            {
                Log(LOG_ERR, "%s: %s", strerror(errno), serverCfg->tlsCert);
                exit = EXIT_FAILURE;
                goto finish;
            }

            if (UInt64Eq(UtilLastModified(serverCfg->tlsKey), UInt64Create(0, 0)))
            {
                Log(LOG_ERR, "%s: %s", strerror(errno), serverCfg->tlsKey);
                exit = EXIT_FAILURE;
                goto finish;
            }
        }

        server = HttpServerCreate(serverCfg);
        if (!server)
        {
            Log(LOG_ERR, "Unable to create HTTP server on port %d: %s",
                serverCfg->port, strerror(errno));

            exit = EXIT_FAILURE;
            goto finish;
        }
        ArrayAdd(httpServers, server);
    }

    if (!ArraySize(httpServers))
    {
        Log(LOG_ERR, "No valid HTTP listeners specified in the configuration.");
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

    if (!tConfig->maxCache)
    {
        Log(LOG_WARNING, "Database caching is disabled.");
        Log(LOG_WARNING, "If this is not what you intended, check the config file");
        Log(LOG_WARNING, "and ensure that maxCache is a valid number of bytes.");
    }

    DbMaxCacheSet(matrixArgs.db, tConfig->maxCache);

    ConfigUnlock(tConfig);
    tConfig = NULL;

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

    Log(LOG_NOTICE, "Building routing tree...");
    matrixArgs.router = RouterBuild();
    if (!matrixArgs.router)
    {
        Log(LOG_ERR, "Unable to build routing tree.");
        exit = EXIT_FAILURE;
        goto finish;
    }

    Log(LOG_NOTICE, "Starting server...");

    for (i = 0; i < ArraySize(httpServers); i++)
    {
        HttpServerConfig *serverCfg;

        server = ArrayGet(httpServers, i);
        serverCfg = HttpServerConfigGet(server);

        if (!HttpServerStart(server))
        {
            Log(LOG_ERR, "Unable to start HTTP server %lu on port %hu.", i, serverCfg->port);
            exit = EXIT_FAILURE;
            goto finish;
        }
        else
        {
            Log(LOG_DEBUG, "Started HTTP server %lu.", i);
            Log(LOG_INFO, "Listening on port: %hu", serverCfg->port);
        }
    }


    sigAction.sa_handler = SignalHandler;
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART;

#define SIGACTION(sig, act, oact) \
    if (sigaction(sig, act, oact) < 0) \
    { \
        Log(LOG_ERR, "Unable to install signal handler: %s", #sig); \
        exit = EXIT_FAILURE; \
        goto finish; \
    } \
    else \
    { \
        Log(LOG_DEBUG, "Installed signal handler: %s", #sig); \
    }

    SIGACTION(SIGINT, &sigAction, NULL);
    SIGACTION(SIGTERM, &sigAction, NULL);
    SIGACTION(SIGPIPE, &sigAction, NULL);
    SIGACTION(SIGUSR1, &sigAction, NULL);

#undef SIGACTION

    /* Block this thread until the servers are terminated by a signal
     * handler */
    for (i = 0; i < ArraySize(httpServers); i++)
    {
        server = ArrayGet(httpServers, i);
        HttpServerJoin(server);
        Log(LOG_DEBUG, "Joined HTTP server %lu.", i);
    }

finish:
    Log(LOG_NOTICE, "Shutting down...");
    if (httpServers)
    {
        for (i = 0; i < ArraySize(httpServers); i++)
        {
            Log(LOG_DEBUG, "Freeing HTTP server %lu...", i);
            server = ArrayGet(httpServers, i);
            HttpServerStop(server);
            HttpServerFree(server);
            Log(LOG_DEBUG, "Freed HTTP server %lu.", i);
        }
        ArrayFree(httpServers);
        httpServers = NULL;

        Log(LOG_DEBUG, "Freed HTTP servers array.");
    }

    if (cron)
    {
        Log(LOG_DEBUG, "Waiting on background jobs...");
        CronStop(cron);
        CronFree(cron);
        Log(LOG_DEBUG, "Stopped and freed job scheduler.");
    }

    ConfigUnlock(tConfig);
    Log(LOG_DEBUG, "Unlocked configuration.");

    DbClose(matrixArgs.db);
    Log(LOG_DEBUG, "Closed database.");

    HttpRouterFree(matrixArgs.router);
    Log(LOG_DEBUG, "Freed routing tree.");

    /*
     * Uninstall the memory hook because it uses the Log
     * API, whose configuration is being freed now, so it
     * won't work anymore.
     */
    MemoryHook(NULL, NULL);

    StreamClose(logFile);

    if (restart)
    {
        /*
         * Change back into starting directory so initial chdir()
         * call works.
         */
        if (chdir(startDir) != 0)
        {
            /* TODO: Seems problematic, what do we do? */
        }
        goto start;
    }

    return exit;
}
