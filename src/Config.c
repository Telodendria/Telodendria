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
#include <Array.h>
#include <Str.h>
#include <Db.h>
#include <HttpServer.h>
#include <Log.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#define CONFIG_REQUIRE(key, type) \
    value = HashMapGet(config, key); \
    if (!value) \
    { \
        tConfig->err = "Missing required " key " directive."; \
        goto error; \
    } \
    if (JsonValueType(value) == JSON_NULL) \
    { \
        tConfig->err = "Missing value for " key " directive."; \
        goto error; \
    } \
    if (JsonValueType(value) != type) \
    { \
        tConfig->err = "Expected " key " to be of type " #type; \
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
            tConfig->err = "Expected " key " to be of type JSON_STRING"; \
            goto error; \
        } \
        into = StrDuplicate(JsonValueAsString(value)); \
    } \
    else \
    { \
        into = default ? StrDuplicate(default) : NULL; \
    }

#define CONFIG_OPTIONAL_INTEGER(into, key, default) \
    value = HashMapGet(config, key); \
    if (value && JsonValueType(value) != JSON_NULL) \
    { \
        if (JsonValueType(value) != JSON_INTEGER) \
        { \
            tConfig->err = "Expected " key " to be of type JSON_INTEGER"; \
            goto error; \
        } \
        into = JsonValueAsInteger(value); \
    } \
    else \
    { \
        into = default; \
    }

static int
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

static int
ConfigParseListen(Config * tConfig, Array * listen)
{
    size_t i;

    if (!ArraySize(listen))
    {
        tConfig->err = "Listen array cannot be empty; you must specify at least one listener.";
        goto error;
    }

    if (!tConfig->servers)
    {
        tConfig->servers = ArrayCreate();
        if (!tConfig->servers)
        {
            tConfig->err = "Unable to allocate memory for listener configurations.";
            goto error;
        }
    }

    for (i = 0; i < ArraySize(listen); i++)
    {
        JsonValue *val = ArrayGet(listen, i);
        HashMap *obj;
        HttpServerConfig *serverCfg = Malloc(sizeof(HttpServerConfig));

        if (!serverCfg)
        {
            tConfig->err = "Unable to allocate memory for listener configuration.";
            goto error;
        }

        if (JsonValueType(val) != JSON_OBJECT)
        {
            tConfig->err = "Invalid value in listener array. All listeners must be objects.";
            goto error;
        }

        obj = JsonValueAsObject(val);

        serverCfg->port = JsonValueAsInteger(HashMapGet(obj, "port"));
        serverCfg->threads = JsonValueAsInteger(HashMapGet(obj, "threads"));
        serverCfg->maxConnections = JsonValueAsInteger(HashMapGet(obj, "maxConnections"));

        if (!serverCfg->port)
        {
            Free(serverCfg);
            continue;
        }

        if (!serverCfg->threads)
        {
            serverCfg->threads = 4;
        }

        if (!serverCfg->maxConnections)
        {
            serverCfg->maxConnections = 32;
        }

        val = HashMapGet(obj, "tls");
        if ((JsonValueType(val) == JSON_BOOLEAN && !JsonValueAsBoolean(val)) || JsonValueType(val) == JSON_NULL)
        {
            serverCfg->flags = HTTP_FLAG_NONE;
            serverCfg->tlsCert = NULL;
            serverCfg->tlsKey = NULL;
        }
        else if (JsonValueType(val) != JSON_OBJECT)
        {
            tConfig->err = "Invalid value for listener.tls. It must be an object.";
            goto error;
        }
        else
        {
            serverCfg->flags = HTTP_FLAG_TLS;

            obj = JsonValueAsObject(val);
            serverCfg->tlsCert = StrDuplicate(JsonValueAsString(HashMapGet(obj, "cert")));
            serverCfg->tlsKey = StrDuplicate(JsonValueAsString(HashMapGet(obj, "key")));

            if (!serverCfg->tlsCert || !serverCfg->tlsKey)
            {
                tConfig->err = "TLS cert and key must both be valid file names.";
                goto error;
            }
        }
        ArrayAdd(tConfig->servers, serverCfg);
    }

    return 1;
error:
    return 0;
}

static int
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
        tConfig->err = "Invalid value for log.output";
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
        tConfig->err = "Invalid value for log.level.";
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
            tConfig->err = "Expected type JSON_BOOLEAN for log.color.";
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

    Free(tConfig->logTimestamp);

    if (tConfig->servers)
    {
        size_t i;

        for (i = 0; i < ArraySize(tConfig->servers); i++)
        {
            HttpServerConfig *serverCfg = ArrayGet(tConfig->servers, i);

            Free(serverCfg->tlsCert);
            Free(serverCfg->tlsKey);
            Free(serverCfg);
        }

        ArrayFree(tConfig->servers);
    }

    Free(tConfig);
}

Config *
ConfigParse(HashMap *config)
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

    CONFIG_REQUIRE("listen", JSON_ARRAY);
    if (!ConfigParseListen(tConfig, JsonValueAsArray(value)))
    {
        goto error;
    }

    CONFIG_REQUIRE("serverName", JSON_STRING);
    CONFIG_COPY_STRING(tConfig->serverName);

    value = HashMapGet(config, "baseUrl");
    if (value)
    {
        CONFIG_COPY_STRING(tConfig->baseUrl);
    }
    else
    {
        tConfig->baseUrl = Malloc(strlen(tConfig->serverName) + 10);
        if (!tConfig->baseUrl)
        {
            tConfig->err = "Error allocating memory for default config value 'baseUrl'.";
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
            tConfig->err = "Config directive 'runAs' should be a JSON object that contains a 'uid' and 'gid'.";
            goto error;
        }
    }

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

    tConfig->ok = 1;
    tConfig->err = NULL;
    return tConfig;

error:
    tConfig->ok = 0;
    return tConfig;
}

int
ConfigExists(Db *db)
{
    return DbExists(db, 1, "config");
}

int
ConfigCreateDefault(Db *db)
{
    DbRef *ref;
    HashMap *json;
    Array *listeners;
    HashMap *listen;

    char hostname[HOST_NAME_MAX + 1];

    if (!db)
    {
        return 0;
    }

    ref = DbCreate(db, 1, "config");
    if (!ref)
    {
        return 0;
    }

    json = DbJson(ref);

    JsonSet(json, JsonValueString("file"), 2, "log", "output");

    listeners = ArrayCreate();
    listen = HashMapCreate();
    HashMapSet(listen, "port", JsonValueInteger(8008));
    HashMapSet(listen, "tls", JsonValueBoolean(0));
    ArrayAdd(listeners, JsonValueObject(listen));
    HashMapSet(json, "listen", JsonValueArray(listeners));

    if (gethostname(hostname, HOST_NAME_MAX + 1) < 0)
    {
        strcpy(hostname, "localhost");
    }
    HashMapSet(json, "serverName", JsonValueString(hostname));

    HashMapSet(json, "federation", JsonValueBoolean(1));
    HashMapSet(json, "registration", JsonValueBoolean(0));

    return DbUnlock(db, ref);
}

Config *
ConfigLock(Db *db)
{
    Config *config;
    DbRef *ref = DbLock(db, 1, "config");

    if (!ref)
    {
        return NULL;
    }

    config = ConfigParse(DbJson(ref));
    if (config)
    {
        config->db = db;
        config->ref = ref;
    }

    return config;
}

int
ConfigUnlock(Config *config)
{
    Db *db;
    DbRef *dbRef;

    if (!config)
    {
        return 0;
    }

    db = config->db;
    dbRef = config->ref;

    ConfigFree(config);
    return DbUnlock(db, dbRef);
}
