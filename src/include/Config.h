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

#ifndef TELODENDRIA_CONFIG_H
#define TELODENDRIA_CONFIG_H

/***
 * @Nm Config
 * @Nd Parse the Telodendria configuration into a structure.
 * @Dd April 28 2023
 * @Xr Db Json HttpServer Log
 *
 * .Nm
 * validates and maintains the Telodendria server's configuration data.
 * This API builds on the database and JSON APIs to add parsing
 * specific to Telodendria. It converts a configuration in its raw
 * form into a structure that is much easier and more convenient to
 * work with.
 * .Pp
 * Since very early on in Telodendria's development, the configuration
 * file has existed inside of the database. This API also offers
 * convenience methods for extracting the configuration out of the
 * database.
 * .Pp
 * This documentation does not describe the actual format of the
 * configuration file; for that, consult
 * .Xr telodendria-config 7 .
 */

#include <HashMap.h>
#include <Array.h>
#include <Db.h>

/**
 * Bit flags that can be set in the flags field of the configuration
 * structure.
 */
typedef enum ConfigFlag
{
    CONFIG_FEDERATION = (1 << 0),
    CONFIG_REGISTRATION = (1 << 1),
    CONFIG_LOG_COLOR = (1 << 2),
    CONFIG_LOG_FILE = (1 << 3),
    CONFIG_LOG_STDOUT = (1 << 4),
    CONFIG_LOG_SYSLOG = (1 << 5)
} ConfigFlag;

/**
 * The configuration structure is not opaque like many of the other
 * structures present in the other public APIs. This is intentional;
 * defining functions for all of the fields would simply add too much
 * unnecessary overhead.
 */
typedef struct Config
{
    /*
     * These are used internally and should not be touched outside of
     * the functions defined in this API.
     */
    Db *db;
    DbRef *ref;

    /*
     * Whether or not the parsing was successful. If this boolean
     * value is 0, then read the error message and assume that all
     * other fields are invalid.
     */
    int ok;
    char *err;

    char *serverName;
    char *baseUrl;
    char *identityServer;

    char *uid;
    char *gid;

    unsigned int flags;

    size_t maxCache;

    char *logTimestamp;
    int logLevel;

    /*
     * An array of HttpServerConfig structures. Consult the HttpServer
     * API.
     */
    Array *servers;
} Config;

/**
 * Parse a JSON object, extracting the necessary values, validating
 * them, and adding them to the configuration structure for use by the
 * caller. All values are copied, so the JSON object can be safely
 * freed after this function returns.
 * .Pp
 * If an error occurs, this function will not return NULL, but it will
 * set the ok flag to 0. The caller should always check the ok flag,
 * and if there is an error, it should display the error to the user.
 */
Config * ConfigParse(HashMap *);

/**
 * Free all the values inside of the given configuration structure,
 * as well as the structure itself, such that it is completely invalid
 * when this function returns.
 */
void ConfigFree(Config *);

/**
 * Check whether or not the configuration exists in the database,
 * returning a boolean value to indicate the status.
 */
extern int ConfigExists(Db *);

/**
 * Create a sane default configuration in the specified database.
 * This function returns a boolean value indicating whether or not it
 * was successful.
 */
extern int ConfigCreateDefault(Db *);

/**
 * Lock the configuration in the database using
 * .Fn DbLock ,
 * and then parse the object using
 * .Fn ConfigParse .
 * The return value of this function is the same as
 * .Fn ConfigParse .
 */
extern Config * ConfigLock(Db *);

/**
 * Unlock the specified configuration, returning it back to the
 * database. This function also invalidates all memory associated with
 * this config object, so values that should be retained after this is
 * called should be duplicated as necessary.
 */
extern int ConfigUnlock(Config *);

#endif                             /* TELODENDRIA_CONFIG_H */
