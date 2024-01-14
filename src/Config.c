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
#include <Schema/Config.h>
#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Array.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Db.h>
#include <Cytoplasm/Log.h>
#include <Cytoplasm/Util.h>

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <grp.h>
#include <pwd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif

void
ConfigParse(HashMap * config, Config *tConfig)
{
    size_t i;

    if (!config)
    {
        tConfig->ok = 0;
        tConfig->err = "Invalid object given as config.";
        return;
    }

    memset(tConfig, 0, sizeof(Config));

    tConfig->maxCache = 0;

    if (!ConfigFromJson(config, tConfig, &tConfig->err))
    {
        ConfigFree(tConfig);
        goto error;
    }
    if (!tConfig->baseUrl)
    {
        size_t len = strlen(tConfig->serverName) + 10;

        tConfig->baseUrl = Malloc(len);
        if (!tConfig->baseUrl)
        {
            tConfig->err = "Couldn't allocate enough memory for 'baseUrl'.";
            goto error;
        }
        snprintf(tConfig->baseUrl, len, "https://%s/", tConfig->serverName);
    }
    if (!tConfig->log.timestampFormat)
    {
        tConfig->log.timestampFormat = StrDuplicate("default");
    }
    for (i = 0; i < ArraySize(tConfig->listen); i++)
    {
        ConfigListener *listener = ArrayGet(tConfig->listen, i);
        if (!listener->maxConnections)
        {
            listener->maxConnections = 32;
        }
        if (!listener->threads)
        {
            listener->threads = 4;
        }
        if (!listener->port)
        {
            listener->port = 8008;
        }
    }
    tConfig->ok = 1;
    tConfig->err = NULL;
    return;

error:
    tConfig->ok = 0;
    return;
}

int
ConfigExists(Db * db)
{
    return DbExists(db, 1, "config");
}

int
ConfigCreateDefault(Db * db)
{
    Config config;
    ConfigListener *listener;

    HashMap *json;
    JsonValue *val;

    DbRef *ref;

    size_t len;

    memset(&config, 0, sizeof(Config));


    config.log.output = CONFIG_LOG_OUTPUT_FILE;

    config.runAs.gid = StrDuplicate(getgrgid(getgid())->gr_name);
    config.runAs.uid = StrDuplicate(getpwuid(getuid())->pw_name);

    config.registration = 0;
    config.federation = 1;

    /* Create serverName and baseUrl. */
    config.serverName = Malloc(HOST_NAME_MAX + 1);
    memset(config.serverName, 0, HOST_NAME_MAX + 1);
    gethostname(config.serverName, HOST_NAME_MAX);
    len = strlen(config.serverName) + 10;
    config.baseUrl = Malloc(len);
    snprintf(config.baseUrl, len, "https://%s/", config.serverName);

    /* Add simple listener without TLS. */
    config.listen = ArrayCreate();
    listener = Malloc(sizeof(ConfigListener));
    listener->maxConnections = 32;
    listener->port = 8008;
    listener->threads = 4;

    ArrayAdd(config.listen, listener);

    /* Write it all out to the configuration file. */
    json = ConfigToJson(&config);
    val = JsonGet(json, 1, "listen");
    val = ArrayGet(JsonValueAsArray(val), 0);
    JsonValueFree(HashMapDelete(JsonValueAsObject(val), "tls"));

    ref = DbCreate(db, 1, "config");
    if (!ref)
    {
        ConfigFree(&config);
        return 0;
    }
    DbJsonSet(ref, json);
    DbUnlock(db, ref);

    ConfigFree(&config);
    JsonFree(json);

    return 1;
}

void
ConfigLock(Db * db, Config *config)
{
    DbRef *ref = DbLock(db, 1, "config");

    if (!ref)
    {
        config->ok = 0;
        config->err = "Couldn't lock configuration.";
    }

    ConfigParse(DbJson(ref), config);
    if (config->ok)
    {
        config->db = db;
        config->ref = ref;
    }
}

int
ConfigUnlock(Config *config)
{
    Db *db;
    DbRef *dbRef;

    if (!config->ok)
    {
        return 0;
    }

    db = config->db;
    dbRef = config->ref;

    ConfigFree(config);
    config->ok = 0;

    return DbUnlock(db, dbRef);
}
int 
ConfigLogLevelToSyslog(ConfigLogLevel level)
{
    switch (level)
    {
        case CONFIG_LOG_LEVEL_NOTICE:
            return LOG_NOTICE;
        case CONFIG_LOG_LEVEL_ERROR:
            return LOG_ERR;
        case CONFIG_LOG_LEVEL_MESSAGE:
            return LOG_INFO;
        case CONFIG_LOG_LEVEL_DEBUG:
            return LOG_DEBUG;
        case CONFIG_LOG_LEVEL_WARNING:
            return LOG_WARNING;
    }
    return LOG_INFO;
}
