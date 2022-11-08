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
#include <Memory.h>
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
        if (!isdigit((unsigned char) *str))
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

    tConfig = Malloc(sizeof(TelodendriaConfig));
    if (!tConfig)
    {
        return NULL;
    }

    memset(tConfig, 0, sizeof(TelodendriaConfig));

    directive = (ConfigDirective *) HashMapGet(config, "listen");
    children = ConfigChildrenGet(directive);
    value = ConfigValuesGet(directive);

    if (!directive)
    {
        tConfig->listenPort = 8008;
    }
    else
    {
        ASSERT_NO_CHILDREN("listen");
        ASSERT_VALUES("listen", 1);

        tConfig->listenPort = (unsigned short) atoi(ArrayGet(value, 0));
        if (!tConfig->listenPort)
        {
            Log(lc, LOG_ERROR, "Expected numeric value for listen port, got '%s'.", ArrayGet(value, 1));
            goto error;
        }
    }

    GET_DIRECTIVE("server-name");
    ASSERT_NO_CHILDREN("server-name");
    ASSERT_VALUES("server-name", 1);
    COPY_VALUE(tConfig->serverName, 0);

    directive = (ConfigDirective *) HashMapGet(config, "base-url");
    children = ConfigChildrenGet(directive);
    value = ConfigValuesGet(directive);

    if (directive)
    {
        ASSERT_NO_CHILDREN("base-url");
        ASSERT_VALUES("base-url", 1);
        COPY_VALUE(tConfig->baseUrl, 0);
    }
    else
    {
        Log(lc, LOG_WARNING, "Base URL not specified. Assuming it's 'https://%s'.", tConfig->serverName);
        tConfig->baseUrl = Malloc(strlen(tConfig->serverName) + 10);
        if (!tConfig->baseUrl)
        {
            Log(lc, LOG_ERROR, "Error allocating memory for default config value 'base-url'.");
            goto error;
        }

        sprintf(tConfig->baseUrl, "https://%s", tConfig->serverName);
    }

    directive = (ConfigDirective *) HashMapGet(config, "identity-server");
    children = ConfigChildrenGet(directive);
    value = ConfigValuesGet(directive);

    if (directive)
    {
        ASSERT_NO_CHILDREN("identity-server");
        ASSERT_VALUES("identity-server", 1);
        COPY_VALUE(tConfig->identityServer, 0);
    }
    else
    {
        Log(lc, LOG_WARNING, "Identity server not specified. No identity server will be advertised.");
        tConfig->identityServer = NULL;
    }

    directive = (ConfigDirective *) HashMapGet(config, "id");
    children = ConfigChildrenGet(directive);
    value = ConfigValuesGet(directive);

    ASSERT_NO_CHILDREN("id");

    if (directive)
    {

        switch (ArraySize(value))
        {
            case 1:
                Log(lc, LOG_WARNING, "No run group specified; assuming it's the same as the user.");
                COPY_VALUE(tConfig->uid, 0);
                tConfig->gid = UtilStringDuplicate(tConfig->uid);
                break;
            case 2:
                COPY_VALUE(tConfig->uid, 0);
                COPY_VALUE(tConfig->gid, 1);
                break;
            default:
                Log(lc, LOG_ERROR,
                    "Wrong value count in directive 'id': got '%d', but expected 1 or 2.",
                    ArraySize(value));
                goto error;
        }
    }
    else
    {
        tConfig->uid = NULL;
        tConfig->gid = NULL;
    }

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
        if (!tConfig->threads)
        {
            Log(lc, LOG_ERROR, "threads must be greater than zero");
            goto error;
        }
    }
    else
    {
        Log(lc, LOG_ERROR,
            "Expected integer for directive 'threads', "
            "but got '%s'.", ArrayGet(value, 0));
        goto error;
    }

    directive = (ConfigDirective *) HashMapGet(config, "max-connections");
    if (!directive)
    {
        Log(lc, LOG_WARNING, "max-connections not specified; using defaults, which may change");
        tConfig->maxConnections = 32;
    }
    else
    {
        ASSERT_NO_CHILDREN("max-connections");
        ASSERT_VALUES("max-connections", 1);
        if (IsInteger(ArrayGet(value, 0)))
        {
            tConfig->maxConnections = atoi(ArrayGet(value, 0));
            if (!tConfig->maxConnections)
            {
                Log(lc, LOG_ERROR, "max-connections must be greater than zero.");
                goto error;
            }
        }
        else
        {
            Log(lc, LOG_ERROR, "Expected integer for max-connections, got '%s'", ArrayGet(value, 0));
            goto error;
        }
    }

    GET_DIRECTIVE("max-cache");
    ASSERT_NO_CHILDREN("max-cache");
    ASSERT_VALUES("max-cache", 1);
    tConfig->maxCache = UtilStringToBytes(ArrayGet(value, 0));

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
                tConfig->logLevel = LOG_MESSAGE;
            }
            else if (strcmp(cVal, "debug") == 0)
            {
                tConfig->logLevel = LOG_DEBUG;
            }
            else if (strcmp(cVal, "task") == 0)
            {
                tConfig->logLevel = LOG_TASK;
            }
            else if (strcmp(cVal, "warning") == 0)
            {
                tConfig->logLevel = LOG_WARNING;
            }
            else if (strcmp(cVal, "error") == 0)
            {
                tConfig->logLevel = LOG_ERROR;
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
                tConfig->logTimestamp = NULL;
            }
            else if (strcmp(cVal, "default") != 0)
            {
                tConfig->logTimestamp = UtilStringDuplicate(cVal);
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

            if (strcmp(cVal, "true") == 0)
            {
                tConfig->flags |= TELODENDRIA_LOG_COLOR;
            }
            else if (strcmp(cVal, "false") != 0)
            {
                Log(lc, LOG_ERROR, "Expected boolean value for log.color, got '%s'.", cVal);
                goto error;
            }
        }
    }

    /* Set the actual log output file last */
    if (strcmp(ArrayGet(value, 0), "stdout") == 0)
    {
        tConfig->flags |= TELODENDRIA_LOG_STDOUT;
    }
    else if (strcmp(ArrayGet(value, 0), "file") == 0)
    {
        tConfig->flags |= TELODENDRIA_LOG_FILE;
    }
    else if (strcmp(ArrayGet(value, 0), "syslog") == 0)
    {
        tConfig->flags |= TELODENDRIA_LOG_SYSLOG;
    }
    else
    {
        Log(lc, LOG_ERROR, "Unknown log value '%s', expected 'stdout', 'file', or 'syslog'.",
            ArrayGet(value, 0));
        goto error;
    }

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

    Free(tConfig->serverName);
    Free(tConfig->baseUrl);
    Free(tConfig->identityServer);

    Free(tConfig->uid);
    Free(tConfig->gid);
    Free(tConfig->dataDir);

    Free(tConfig);
}
