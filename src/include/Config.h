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

#include <Schema/Config.h>

#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Array.h>
#include <Cytoplasm/Db.h>

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
extern void ConfigParse(HashMap *, Config *);

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
extern void ConfigLock(Db *, Config *);

/**
 * Unlock the specified configuration, returning it back to the
 * database. This function also invalidates all memory associated with
 * this config object, so values that should be retained after this is
 * called should be duplicated as necessary.
 */
extern int ConfigUnlock(Config *);

/**
 * Converts a ConfigLogLevel into a valid syslog level.
 */
extern int ConfigLogLevelToSyslog(ConfigLogLevel);

#endif                             /* TELODENDRIA_CONFIG_H */
