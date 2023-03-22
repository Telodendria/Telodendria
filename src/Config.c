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
#include <Config.h>
#include <Memory.h>
#include <Json.h>
#include <HashMap.h>
#include <Log.h>
#include <Array.h>
#include <Str.h>
#include <Db.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define CONFIG_REQUIRE(key, type) \
    value = HashMapGet(config, key); \
    if (!value) \
    { \
        Log(LOG_ERR, "Missing required " key " directive."); \
        goto error; \
    } \
    if (JsonValueType(value) == JSON_NULL) \
    { \
        Log(LOG_ERR, "Missing value for " key " directive."); \
        goto error; \
    } \
    if (JsonValueType(value) != type) \
    { \
        Log(LOG_ERR, "Expected " key " to be of type " #type); \
        goto error; \
    }

#define CONFIG_COPY_STRING(into) \
    into = StrDuplicate(JsonValueAsString(value));

#define CONFIG_OPTIONAL_STRING(into, key, default) \
    value = HashMapGet(config, key); \
    if (value && JsonValueType(value) != JSON_NULL) \
    { \
        if (JsonValueType(value) != JSON_STRING) \
        { \
            Log(LOG_ERR, "Expected " key " to be of type JSON_STRING"); \
            goto error; \
        } \
        into = StrDuplicate(JsonValueAsString(value)); \
    } \
    else \
    { \
        Log(LOG_INFO, "Using default value " #default " for " key "."); \
        into = default ? StrDuplicate(default) : NULL; \
    }

#define CONFIG_OPTIONAL_INTEGER(into, key, default) \
    value = HashMapGet(config, key); \
    if (value && JsonValueType(value) != JSON_NULL) \
    { \
        if (JsonValueType(value) != JSON_INTEGER) \
        { \
            Log(LOG_ERR, "Expected " key " to be of type JSON_INTEGER"); \
            goto error; \
        } \
        into = JsonValueAsInteger(value); \
    } \
    else \
    { \
        Log(LOG_INFO, "Using default value " #default " for " key "."); \
        into = default; \
    }

int
ConfigParseRunAs(Config * tConfig, HashMap * config)
{
    JsonValue *value;

    CONFIG_REQUIRE("uid", JSON_STRING);
    CONFIG_COPY_STRING(tConfig->uid);

    CONFIG_OPTIONAL_STRING(tConfig->gid, "gid", tConfig->uid);

    return 1;

error:
    return 0;
}

int
ConfigParseLog(Config * tConfig, HashMap * config)
{
    JsonValue *value;
    char *str;

    CONFIG_REQUIRE("output", JSON_STRING);
    str = JsonValueAsString(value);

    if (strcmp(str, "stdout") == 0)
    {
        tConfig->flags |= CONFIG_LOG_STDOUT;
    }
    else if (strcmp(str, "file") == 0)
    {
        tConfig->flags |= CONFIG_LOG_FILE;
    }
    else if (strcmp(str, "syslog") == 0)
    {
        tConfig->flags |= CONFIG_LOG_SYSLOG;
    }
    else
    {
        Log(LOG_ERR, "Invalid value for log.output: '%s'.", str);
        goto error;
    }

    CONFIG_OPTIONAL_STRING(str, "level", "message");

    if (strcmp(str, "message") == 0)
    {
        tConfig->logLevel = LOG_INFO;
    }
    else if (strcmp(str, "debug") == 0)
    {
        tConfig->logLevel = LOG_DEBUG;
    }
    else if (strcmp(str, "notice") == 0)
    {
        tConfig->logLevel = LOG_NOTICE;
    }
    else if (strcmp(str, "warning") == 0)
    {
        tConfig->logLevel = LOG_WARNING;
    }
    else if (strcmp(str, "error") == 0)
    {
        tConfig->logLevel = LOG_ERR;
    }
    else
    {
        Log(LOG_ERR, "Invalid value for log.level: '%s'.", tConfig->logLevel);
        goto error;
    }

    Free(str);

    CONFIG_OPTIONAL_STRING(tConfig->logTimestamp, "timestampFormat", "default");

    if (strcmp(tConfig->logTimestamp, "none") == 0)
    {
        Free(tConfig->logTimestamp);
        tConfig->logTimestamp = NULL;
    }

    value = HashMapGet(config, "color");
    if (value && JsonValueType(value) != JSON_NULL)
    {
        if (JsonValueType(value) != JSON_BOOLEAN)
        {
            Log(LOG_ERR, "Expected type JSON_BOOLEAN for log.color.");
            goto error;
        }

        if (JsonValueAsBoolean(value))
        {
            tConfig->flags |= CONFIG_LOG_COLOR;
        }
    }

    return 1;

error:
    return 0;
}

Config *
ConfigParse(HashMap * config)
{
    Config *tConfig;
    JsonValue *value;

    if (!config)
    {
        return NULL;
    }

    tConfig = Malloc(sizeof(Config));
    if (!tConfig)
    {
        return NULL;
    }

    memset(tConfig, 0, sizeof(Config));

    CONFIG_OPTIONAL_INTEGER(tConfig->listenPort, "listen", 8008);

    CONFIG_REQUIRE("serverName", JSON_STRING);
    CONFIG_COPY_STRING(tConfig->serverName);

    value = HashMapGet(config, "baseUrl");
    if (value)
    {
        CONFIG_COPY_STRING(tConfig->baseUrl);
    }
    else
    {
        Log(LOG_WARNING, "Base URL not specified. Assuming it's 'https://%s'.", tConfig->serverName);
        tConfig->baseUrl = Malloc(strlen(tConfig->serverName) + 10);
        if (!tConfig->baseUrl)
        {
            Log(LOG_ERR, "Error allocating memory for default config value 'baseUrl'.");
            goto error;
        }

        sprintf(tConfig->baseUrl, "https://%s", tConfig->serverName);
    }

    CONFIG_OPTIONAL_STRING(tConfig->identityServer, "identityServer", NULL);

    value = HashMapGet(config, "runAs");
    if (value && JsonValueType(value) != JSON_NULL)
    {
        if (JsonValueType(value) == JSON_OBJECT)
        {
            if (!ConfigParseRunAs(tConfig, JsonValueAsObject(value)))
            {
                goto error;
            }
        }
        else
        {
            Log(LOG_ERR, "Config directive 'runAs' should be a JSON object");
            Log(LOG_ERR, "that contains a 'uid' and 'gid'.");
            goto error;
        }
    }

    CONFIG_REQUIRE("dataDir", JSON_STRING);
    CONFIG_COPY_STRING(tConfig->dataDir);

    CONFIG_OPTIONAL_INTEGER(tConfig->threads, "threads", 1);
    CONFIG_OPTIONAL_INTEGER(tConfig->maxConnections, "maxConnections", 32);
    CONFIG_OPTIONAL_INTEGER(tConfig->maxCache, "maxCache", 0);

    CONFIG_REQUIRE("federation", JSON_BOOLEAN);
    if (JsonValueAsBoolean(value))
    {
        tConfig->flags |= CONFIG_FEDERATION;
    }

    CONFIG_REQUIRE("registration", JSON_BOOLEAN);
    if (JsonValueAsBoolean(value))
    {
        tConfig->flags |= CONFIG_REGISTRATION;
    }

    CONFIG_REQUIRE("log", JSON_OBJECT);
    if (!ConfigParseLog(tConfig, JsonValueAsObject(value)))
    {
        goto error;
    }

    return tConfig;

error:
    ConfigFree(tConfig);
    return NULL;
}

void
ConfigFree(Config * tConfig)
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

    Free(tConfig->logTimestamp);

    Free(tConfig);
}
