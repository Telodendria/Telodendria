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
#include <Log.h>

#include <Memory.h>

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>

#define LOG_TSBUFFER 64

struct LogConfig
{
    int level;
    size_t indent;
    FILE *out;
    int flags;
    char *tsFmt;

    pthread_mutex_t lock;
};

LogConfig *
LogConfigCreate(void)
{
    LogConfig *config;

    config = Malloc(sizeof(LogConfig));

    if (!config)
    {
        return NULL;
    }

    memset(config, 0, sizeof(LogConfig));

    LogConfigLevelSet(config, LOG_INFO);
    LogConfigIndentSet(config, 0);
    LogConfigOutputSet(config, NULL);   /* Will set to stdout */
    LogConfigFlagSet(config, LOG_FLAG_COLOR);
    LogConfigTimeStampFormatSet(config, "%y-%m-%d %H:%M:%S");

    return config;
}

void
LogConfigFlagClear(LogConfig * config, int flags)
{
    if (!config)
    {
        return;
    }

    config->flags &= ~flags;
}

static int
LogConfigFlagGet(LogConfig * config, int flags)
{
    if (!config)
    {
        return 0;
    }

    return config->flags & flags;
}

void
LogConfigFlagSet(LogConfig * config, int flags)
{
    if (!config)
    {
        return;
    }

    config->flags |= flags;
}

void
LogConfigFree(LogConfig * config)
{
    if (!config)
    {
        return;
    }

    fclose(config->out);
    Free(config);
}

void
LogConfigIndent(LogConfig * config)
{
    if (config)
    {
        config->indent += 2;
    }
}

void
LogConfigIndentSet(LogConfig * config, size_t indent)
{
    if (!config)
    {
        return;
    }

    config->indent = indent;
}

int
LogConfigLevelGet(LogConfig * config)
{
    if (!config)
    {
        return -1;
    }

    return config->level;
}

void
LogConfigLevelSet(LogConfig * config, int level)
{
    if (!config)
    {
        return;
    }

    switch (level)
    {
        case LOG_ERR:
        case LOG_WARNING:
        case LOG_INFO:
        case LOG_DEBUG:
            config->level = level;
        default:
            break;
    }
}

void
LogConfigOutputSet(LogConfig * config, FILE * out)
{
    if (!config)
    {
        return;
    }

    if (out)
    {
        config->out = out;
    }
    else
    {
        config->out = stdout;
    }

}

void
LogConfigTimeStampFormatSet(LogConfig * config, char *tsFmt)
{
    if (config)
    {
        config->tsFmt = tsFmt;
    }
}

void
LogConfigUnindent(LogConfig * config)
{
    if (config && config->indent >= 2)
    {
        config->indent -= 2;
    }
}

void
Log(LogConfig * config, int level, const char *msg,...)
{
    size_t i;
    int doColor;
    char indicator;
    va_list argp;

    /*
     * Only proceed if we have a config and its log level is set to a
     * value that permits us to log. This is as close as we can get
     * to a no-op function if we aren't logging anything, without doing
     * some crazy macro magic.
     */
    if (!config || level > config->level)
    {
        return;
    }

    /* Misconfiguration */
    if (!config->out)
    {
        return;
    }

    pthread_mutex_lock(&config->lock);

    if (LogConfigFlagGet(config, LOG_FLAG_SYSLOG))
    {
        /* No further print logic is needed; syslog will handle it all
         * for us. */
        va_start(argp, msg);
        vsyslog(level, msg, argp);
        va_end(argp);
        pthread_mutex_unlock(&config->lock);
        return;
    }

    doColor = LogConfigFlagGet(config, LOG_FLAG_COLOR)
            && isatty(fileno(config->out));

    if (doColor)
    {
        char *ansi;

        switch (level)
        {
            case LOG_EMERG:
            case LOG_ALERT:
            case LOG_CRIT:
            case LOG_ERR:
                /* Bold Red */
                ansi = "\033[1;31m";
                break;
            case LOG_WARNING:
                /* Bold Yellow */
                ansi = "\033[1;33m";
                break;
            case LOG_NOTICE:
                /* Bold Magenta */
                ansi = "\033[1;35m";
                break;
            case LOG_INFO:
                /* Bold Green */
                ansi = "\033[1;32m";
                break;
            case LOG_DEBUG:
                /* Bold Blue */
                ansi = "\033[1;34m";
                break;
            default:
                ansi = "";
                break;
        }

        fputs(ansi, config->out);
    }

    fputc('[', config->out);

    if (config->tsFmt)
    {
        time_t timer = time(NULL);
        struct tm *timeInfo = localtime(&timer);
        char tsBuffer[LOG_TSBUFFER];

        int tsLength = strftime(tsBuffer, LOG_TSBUFFER, config->tsFmt,
                                timeInfo);

        if (tsLength)
        {
            fputs(tsBuffer, config->out);
            if (!isspace((unsigned char) tsBuffer[tsLength - 1]))
            {
                fputc(' ', config->out);
            }
        }
    }

    switch (level)
    {
        case LOG_EMERG:
            indicator = '#';
            break;
        case LOG_ALERT:
            indicator = '@';
            break;
        case LOG_CRIT:
            indicator = 'X';
            break;
        case LOG_ERR:
            indicator = 'x';
            break;
        case LOG_WARNING:
            indicator = '!';
            break;
        case LOG_NOTICE:
            indicator = '~';
            break;
        case LOG_INFO:
            indicator = '>';
            break;
        case LOG_DEBUG:
            indicator = '*';
            break;
        default:
            indicator = '?';
            break;
    }

    fprintf(config->out, "%c]", indicator);

    if (doColor)
    {
        /* ANSI Reset */
        fputs("\033[0m", config->out);
    }

    fputc(' ', config->out);
    for (i = 0; i < config->indent; i++)
    {
        fputc(' ', config->out);
    }

    va_start(argp, msg);
    vfprintf(config->out, msg, argp);
    fputc('\n', config->out);
    va_end(argp);

    /* If we are debugging, there might be something that's going to
     * segfault the program coming up, so flush the output stream
     * immediately. */
    if (config->level == LOG_DEBUG)
    {
        fflush(config->out);
    }

    pthread_mutex_unlock(&config->lock);
}
