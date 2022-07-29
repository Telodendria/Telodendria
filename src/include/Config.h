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

/*
 * Config.h: A heavily-modified version of Conifer2, a configuration
 * file format specification and C parsing library written by Jordan
 * Bancino. This library differs from Conifer2 in that the function
 * naming convention has been updated to be consistent with Telodendria,
 * and the underlying data structures have been overhauled to use the
 * data structure libraries provided by Telodendria.
 *
 * Conifer2 was originally a learning project. It was very thoroughly
 * debugged, however, and the configuration syntax was elegant,
 * certainly more elegant than using JSON for a configuration file,
 * so it was chosen to be the format for Telodendria's configuration
 * file. The original Conifer2 project is now dead; Conifer2 lives on
 * only as Telodendria's Config parsing library.
 */
#ifndef TELODENDRIA_CONFIG_H
#define TELODENDRIA_CONFIG_H

#include <stdio.h>

#include <HashMap.h>
#include <Array.h>

/*
 * A configuration directive is a single key that may have at least one
 * value, and any number of children.
 */
typedef struct ConfigDirective ConfigDirective;

/*
 * The parser returns a parse result object. This stores whether or
 * not the parse was successful, and then also additional information
 * about the parse, such as the line number on which parsing failed,
 * or the collection of directives if the parsing succeeded.
 *
 * There are a number of ConfigParseResult methods that can be used
 * to query the result of parsing.
 */
typedef struct ConfigParseResult ConfigParseResult;

/*
 * Parse a configuration file, and generate the structures needed to
 * make it easy to read.
 *
 * Params:
 *
 *   (FILE *) The input stream to read from.
 *
 * Return: A ConfigParseResult, which can be used to check whether or
 * not the parsing was successful. If the parsing was sucessful, then
 * this object contains the root directive, which can be used to
 * retrieve configuration values out of. If the parsing failed, then
 * this object contains the line number at which the parsing was
 * aborted.
 */
extern ConfigParseResult *
 ConfigParse(FILE *);

/*
 * Get whether or not a parse result indicates that parsing was
 * successful or not. This function should be used to determine what
 * to do next. If the parsing failed, your program should terminate
 * with an error, otherwise, you can proceed to parse the configuration
 * file.
 *
 * Params:
 *
 *   (ConfigParseResult *) The output of ConfigParse() to check.
 *
 * Return: 0 if the configuration file is malformed, or otherwise
 * could not be parsed. Any non-zero return value indicates that the
 * configuration file was successfully parsed.
 */
extern unsigned int
 ConfigParseResultOk(ConfigParseResult *);

/*
 * If, and only if, the configuration file parsing failed, then this
 * function can be used to get the line number it failed at. Typically,
 * this will be reported to the user and then the program will be
 * terminated.
 *
 * Params:
 *
 *   (ConfigParseResult *) The output of ConfigParse() to get the
 *                         line number from.
 *
 * Return: The line number on which the configuration file parser
 * choked, or 0 if the parsing was actually successful.
 */
extern size_t
 ConfigParseResultLineNumber(ConfigParseResult *);

/*
 * Convert a ConfigParseResult into a HashMap containing the entire
 * configuration file, if, and only if, the parsing was successful.
 *
 * Params:
 *
 *   (ConfigParseResult *) The output of ConfigParse() to get the
 *                         actual configuration data from.
 *
 * Return: A HashMap containing all the configuration data, or NULL
 * if the parsing was not successful. This HashMap is a map of string
 * keys to ConfigDirective objects. Use the standard HashMap methods
 * to get ConfigDirectives, and then use the ConfigDirective functions
 * to get information out of them.
 */
extern HashMap *
 ConfigParseResultGet(ConfigParseResult *);

/*
 * Free the memory being used by the given ConfigParseResult. Note that
 * it is safe to free the ConfigParseResult immediately after you have
 * retrieved either the line number or the configuration data from it.
 * Freeing the parse result does not free the configuration data.
 *
 * Params:
 *
 *   (ConfigParseResult *) The output of ConfigParse() to free. This
 *                         object will be invalidated, but pointers to
 *                         the actual configuration data will still be
 *                         valid.
 */
extern void
 ConfigParseResultFree(ConfigParseResult *);

/*
 * Get an array of values associated with the given configuration
 * directive. Directives can have any number of values, which are
 * made accessible via the Array API.
 *
 * Params:
 *
 *   (ConfigDirective *) The configuration directive to get the values
 *                       for.
 *
 * Return: An array that contains at least 1 value. Configuration files
 * cannot have value-less directives. If the passed directive is NULL,
 * or there is an error allocating memory for an array, then NULL is
 * returned.
 */
extern Array *
 ConfigValuesGet(ConfigDirective *);

/*
 * Get a map of children associated with the given configuration
 * directive. Configuration files can recurse with no practical limit,
 * so directives can have any number of child directives.
 *
 * Params:
 *
 *   (ConfigDirective *) The configuratio ndirective to get the
 *                       children of.
 *
 * Return: A HashMap containing child directives, or NULL if the passed
 * directive is NULL or has no children.
 */
extern HashMap *
 ConfigChildrenGet(ConfigDirective *);

/*
 * Free all the memory associated with the given configuration hash
 * map. Note: this will free *everything*. All Arrays, HashMaps,
 * ConfigDirectives, and even strings will be invalidated. As such,
 * this should be done after you either copy the values you want, or
 * are done using them. It is highly recommended to use this function
 * near the end of your program's execution during cleanup, otherwise
 * copy any values you need into your own buffers.
 *
 * Note that this should only be run on the root configuration object,
 * not any children. Running on children will produce undefined
 * behavior. This function is recursive; it will get all the children
 * under it.
 *
 * Params:
 *
 *   (HashMap *) The configuration data to free.
 *
 */
extern void
 ConfigFree(HashMap *);

#endif                             /* TELODENDRIA_CONFIG_H */
