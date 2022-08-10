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
#include <TelodendriaConfig.h>
#include <Config.h>
#include <HashMap.h>
#include <Log.h>
#include <Array.h>
#include <Util.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static int
IsInteger(char *str)
{
    while (*str)
    {
        if (!isdigit(*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

#define GET_DIRECTIVE(name) \
	directive = (ConfigDirective *) HashMapGet(config, name); \
	if (!directive) { \
		Log(lc, LOG_ERROR, "Missing required configuration directive: '%s'.", name); \
		goto error; \
	} \
	children = ConfigChildrenGet(directive); \
	value = ConfigValuesGet(directive); \

#define ASSERT_NO_CHILDREN(name) if (children) { \
	Log(lc, LOG_ERROR, "Unexpected child values in directive: '%s'.", name); \
	goto error; \
}

#define ASSERT_VALUES(name, expected) if (ArraySize(value) != expected) { \
	Log(lc, LOG_ERROR, \
		"Wrong value count in directive '%s': got '%d', but expected '%d'.", \
		name, ArraySize(value), expected); \
	goto error; \
}

#define COPY_VALUE(into, index) into = UtilStringDuplicate(ArrayGet(value, index))

TelodendriaConfig *
TelodendriaConfigParse(HashMap * config, LogConfig * lc)
{
    TelodendriaConfig *tConfig;

    ConfigDirective *directive;
    Array *value;
    HashMap *children;

    if (!config || !lc)
    {
        return NULL;
    }

    tConfig = calloc(1, sizeof(TelodendriaConfig));
    if (!tConfig)
    {
        return NULL;
    }

    directive = (ConfigDirective *) HashMapGet(config, "listen");
    children = ConfigChildrenGet(directive);
    value = ConfigValuesGet(directive);

    if (!directive)
    {
        Log(lc, LOG_WARNING, "No 'listen' directive specified; using defaults, which may change.");
        tConfig->listenHost = UtilStringDuplicate("localhost");
        tConfig->listenPort = UtilStringDuplicate("8008");
    }
    else
    {
        ASSERT_NO_CHILDREN("listen");
        ASSERT_VALUES("listen", 2);
        COPY_VALUE(tConfig->listenHost, 0);
        COPY_VALUE(tConfig->listenPort, 1);
    }

    GET_DIRECTIVE("server-name");
    ASSERT_NO_CHILDREN("server-name");
    ASSERT_VALUES("server-name", 1);
    COPY_VALUE(tConfig->serverName, 0);

    GET_DIRECTIVE("chroot");
    ASSERT_NO_CHILDREN("chroot");
    ASSERT_VALUES("chroot", 1);
    COPY_VALUE(tConfig->chroot, 0);

    GET_DIRECTIVE("id");
    ASSERT_NO_CHILDREN("id");
    ASSERT_VALUES("id", 2);
    COPY_VALUE(tConfig->uid, 0);
    COPY_VALUE(tConfig->gid, 1);

    GET_DIRECTIVE("data-dir");
    ASSERT_NO_CHILDREN("data-dir");
    ASSERT_VALUES("data-dir", 1);
    COPY_VALUE(tConfig->dataDir, 0);

    GET_DIRECTIVE("threads");
    ASSERT_NO_CHILDREN("threads");
    ASSERT_VALUES("threads", 1);

    if (IsInteger(ArrayGet(value, 0)))
    {
        tConfig->threads = atoi(ArrayGet(value, 0));
    }
    else
    {
        Log(lc, LOG_ERROR,
            "Expected integer for directive 'threads', "
            "but got '%s'.", ArrayGet(value, 0));
        goto error;
    }

    GET_DIRECTIVE("federation");
    ASSERT_NO_CHILDREN("federation");
    ASSERT_VALUES("federation", 1);

    if (strcmp(ArrayGet(value, 0), "true") == 0)
    {
        tConfig->flags |= TELODENDRIA_FEDERATION;
    }
    else if (strcmp(ArrayGet(value, 0), "false") != 0)
    {
        Log(lc, LOG_ERROR,
            "Expected boolean value for directive 'federation', "
            "but got '%s'.", ArrayGet(value, 0));
        goto error;
    }

    GET_DIRECTIVE("registration");
    ASSERT_NO_CHILDREN("registration");
    ASSERT_VALUES("registration", 1);
    if (strcmp(ArrayGet(value, 0), "true") == 0)
    {
        tConfig->flags |= TELODENDRIA_REGISTRATION;
    }
    else if (strcmp(ArrayGet(value, 0), "false") != 0)
    {
        Log(lc, LOG_ERROR,
            "Expected boolean value for directive 'registration', "
            "but got '%s'.", ArrayGet(value, 0));
        goto error;
    }

    GET_DIRECTIVE("log");
    ASSERT_VALUES("log", 1);

    if (children)
    {
        ConfigDirective *cDirective;
        char *cVal;
        size_t size;

        cDirective = HashMapGet(children, "level");
        if (cDirective)
        {
            size = ArraySize(ConfigValuesGet(cDirective));
            if (size > 1)
            {
                Log(lc, LOG_ERROR, "Expected 1 value for log.level, got %d.", size);
                goto error;
            }

            cVal = ArrayGet(ConfigValuesGet(cDirective), 0);
            if (strcmp(cVal, "message") == 0)
            {
                LogConfigLevelSet(lc, LOG_MESSAGE);
            }
            else if (strcmp(cVal, "debug") == 0)
            {
                LogConfigLevelSet(lc, LOG_DEBUG);
            }
            else if (strcmp(cVal, "task") == 0)
            {
                LogConfigLevelSet(lc, LOG_TASK);
            }
            else if (strcmp(cVal, "warning") == 0)
            {
                LogConfigLevelSet(lc, LOG_WARNING);
            }
            else if (strcmp(cVal, "error") == 0)
            {
                LogConfigLevelSet(lc, LOG_ERROR);
            }
            else
            {
                Log(lc, LOG_ERROR, "Invalid value for log.level: '%s'.", cVal);
                goto error;
            }
        }

        cDirective = HashMapGet(children, "timestampFormat");
        if (cDirective)
        {
            size = ArraySize(ConfigValuesGet(cDirective));
            if (size > 1)
            {
                Log(lc, LOG_ERROR, "Expected 1 value for log.level, got %d.", size);
                goto error;
            }

            cVal = ArrayGet(ConfigValuesGet(cDirective), 0);

            if (strcmp(cVal, "none") == 0)
            {
                LogConfigTimeStampFormatSet(lc, NULL);
            }
            else if (strcmp(cVal, "default") != 0)
            {
                LogConfigTimeStampFormatSet(lc, UtilStringDuplicate(cVal));
            }
        }

        cDirective = HashMapGet(children, "color");
        if (cDirective)
        {
            size = ArraySize(ConfigValuesGet(cDirective));
            if (size > 1)
            {
                Log(lc, LOG_ERROR, "Expected 1 value for log.level, got %d.", size);
                goto error;
            }

            cVal = ArrayGet(ConfigValuesGet(cDirective), 0);

            if (strcmp(cVal, "false") == 0)
            {
                LogConfigFlagClear(lc, LOG_FLAG_COLOR);
            }
            else if (strcmp(cVal, "true") != 0)
            {
                Log(lc, LOG_ERROR, "Expected boolean value for log.color, got '%s'.", cVal);
                goto error;
            }
        }
    }

    /* Set the actual log output file last */
    if (strcmp(ArrayGet(value, 0), "stdout") != 0)
    {
        FILE *out = fopen(ArrayGet(value, 0), "w");

        if (!out)
        {
            Log(lc, LOG_ERROR, "Unable to open log file '%s' for writing.",
                ArrayGet(value, 0));
            goto error;
        }

        Log(lc, LOG_DEBUG, "Redirecting output to '%s'.", ArrayGet(value, 0));
        LogConfigOutputSet(lc, out);
    }

    tConfig->logConfig = lc;
    return tConfig;
error:
    TelodendriaConfigFree(tConfig);
    return NULL;
}

#undef GET_DIRECTIVE
#undef ASSERT_NO_CHILDREN
#undef ASSERT_VALUES

void
TelodendriaConfigFree(TelodendriaConfig * tConfig)
{
    if (!tConfig)
    {
        return;
    }

    free(tConfig->listenHost);
    free(tConfig->listenPort);
    free(tConfig->serverName);
    free(tConfig->chroot);
    free(tConfig->uid);
    free(tConfig->gid);
    free(tConfig->dataDir);

    LogConfigFree(tConfig->logConfig);

    free(tConfig);
}