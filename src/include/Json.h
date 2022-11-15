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
 * Json.h: A fully-featured JSON API for C using Arrays and HashMaps.
 * This API builds on the foundations of Arrays and HashMaps, because
 * that's all a JSON object really is. It provides a JsonValue, which
 * is used to encapsulate arbitrary values while being able to identify
 * them in the future, so that JSON can be effectively handled.
 *
 * This implementation is just to get the job done in parsing and
 * generating JSON. It is extremely strict; it will fail on syntax
 * errors. This is fine for Matrix, because we can just return
 * M_BAD_JSON anything in here fails.
 *
 * One thing to note about this implementation is that it focuses
 * primarily on serialization and deserialization to and from streams.
 * What this means is that it does not provide facilities for handling
 * JSON strings; it only writes JSON to output streams, and reading
 * them from input streams. Of course, you could use the POSIX
 * fmemopen() and open_memstream() functions if you really want to deal
 * with JSON strings, but JSON is intended to be an exchange format.
 * Data should be converted to JSON when it is leaving, and converted
 * from JSON when it is coming in. Ideally, most of the program would
 * have no idea what JSON actually is.
 */
#ifndef TELODENDRIA_JSON_H
#define TELODENDRIA_JSON_H

#include <HashMap.h>
#include <Array.h>

#include <stdio.h>
#include <stddef.h>

/*
 * All the possible JSON types. This enumeration is used to identify
 * the type of the value stored in a JsonValue.
 */
typedef enum JsonType
{
    JSON_NULL,                     /* Maps to nothing. */
    JSON_OBJECT,                   /* Maps to a HashMap of JsonValues */
    JSON_ARRAY,                    /* Maps to an Array of JsonValues */
    JSON_STRING,                   /* Maps to a C string */
    JSON_INTEGER,                  /* Maps to a C long */
    JSON_FLOAT,                    /* Maps to a C double */
    JSON_BOOLEAN                   /* Maps to a C 1 or 0 */
} JsonType;

/*
 * A JsonValue encapsulates all the possible values that can be stored
 * in a JSON object as a single type, so as to provide a consistent
 * API for accessing and setting them. It is an opaque structure that
 * can be managed entirely by the functions defined in this API.
 *
 * Note that in the case of objects, arrays, and strings, this structure
 * only stores pointers to allocated data, it doesn't store the data
 * itself. JsonValues only store integers, floats, booleans, and NULL
 * in their memory. Anything else must be freed separately.
 */
typedef struct JsonValue JsonValue;

/*
 * Get the type of a JsonValue.
 *
 * Params:
 *
 *   (JsonValue *) The value to get the type of.
 *
 * Return: A JsonType that tells what the provided value is, or
 * JSON_NULL if the passed value is NULL. Note that even a fully
 * valid JsonValue may still be of type JSON_NULL, so this function
 * should not be used to check whether or not the JSON value is valid.
 */
extern JsonType
 JsonValueType(JsonValue *);

/*
 * Wrap a HashMap into a JsonValue that represents a JSON object. Note
 * that the HashMap should contain only JsonValues. Any other contents
 * are not supported and will lead to undefined behavior.
 *
 * Params:
 *
 *   (HashMap *) The hash map of JsonValues to wrap in a JsonValue.
 *
 * Return: A JsonValue that holds a pointer to the given object, or
 * NULL if there was an error allocating memory.
 */
extern JsonValue *
 JsonValueObject(HashMap *);

/*
 * Get a HashMap from a JsonValue that represents a JSON object.
 *
 * Params:
 *
 *   (JsonValue *) The value to extract the object from.
 *
 * Return: A HashMap of JsonValues, or NULL if no value was provided,
 * or the value is not of type JSON_OBJECT.
 */
extern HashMap *
 JsonValueAsObject(JsonValue *);

/*
 * The following methods very closely resemble the ones above, and
 * behave pretty much the exact same. To save on time and effort,
 * I'm choosing not to explicitly document all of these. If something
 * is unclear about how these functions work, consult the source code,
 * and then feel free to write the documentation yourself.
 *
 * Otherwise, reach out to the official Matrix room, and someone will
 * be able to help you.
 */

extern JsonValue *
 JsonValueArray(Array * array);

extern Array *
 JsonValueAsArray(JsonValue * value);

extern JsonValue *
 JsonValueString(char *string);

extern char *
 JsonValueAsString(JsonValue *);

extern JsonValue *
 JsonValueInteger(long integer);

extern JsonValue *
 JsonValueFloat(double floating);

extern JsonValue *
 JsonValueBoolean(int boolean);

/*
 * Create a JsonValue that represents a JSON null. Because Arrays and
 * HashMaps should not contain NULL values, I thought it appropriate
 * to provide support for JSON nulls. Yes, a small amount of memory is
 * allocated just to point to a NULL, but this keeps all the APIs
 * clean.
 *
 * Return: A JsonValue that represents a JSON null, or NULL if memory
 * could not be allocated.
 */
extern JsonValue *
 JsonValueNull(void);

/*
 * Free the memory being used by a JSON value. Note that this will
 * recursively free all Arrays, HashMaps, and other JsonValues that
 * are reachable from this one. It will invoke free() on strings as
 * well, so make sure passed string pointers point to strings on the
 * heap, not the stack. This will be the case for all strings returned
 * by JsonDecode(), which is why this assumption is made. However, if
 * you are manually creating JsonObjects and stitching them together,
 * you'll have to manually free them as well. Calling this on a
 * JsonValue that contains a pointer to a stack string is undefined.
 *
 * Params:
 *
 *   (JsonValue *) The JsonValue to recursively free.
 */
extern void
 JsonValueFree(JsonValue *);

/*
 * Recursively free a HashMap of JsonValues. This iterates over all
 * the JsonValues in a HashMap and frees them using JsonValueFree(),
 * which will in turn call JsonFree() on values of type JSON_OBJECT.
 *
 * Params:
 *
 *   (HashMap *) The hash map of JsonValues to recursively free.
 */
extern void
 JsonFree(HashMap *);

/*
 * Encode the given string in such a way that it can be embedded in a
 * JSON stream. This entails:
 *
 *   - Escaping quotes, backslashes, and other special characters using
 *     their backslash escape
 *   - Encoding bytes that are not UTF-8 using \u escapes.
 *   - Wrapping the entire string in double quotes.
 *
 * This function is provided via the public API so it is accessible to
 * custom JSON encoders, such as the CanonicalJson API. This will
 * typically be used for encoding JSON keys; for values, just use
 * JsonEncodeValue().
 *
 * Params:
 *
 *   (const char *) The C string to serialize as a JSON string.
 *   (FILE *)       The output stream to write the encoded string to.
 */
extern void
 JsonEncodeString(const char *, FILE *);

/*
 * Serialize a JsonValue as it would appear in JSON output. This is
 * a recursive function that will also encode all child values
 * reachable from the given JsonValue.
 *
 * This is exposed via the public API so that custom JSON encoders
 * such as CanonicalJson can take advantage of it. Normal users that
 * are writing custom encoders should just use JsonEncode() to encode
 * an entire object.
 *
 * Params:
 *
 *   (JsonValue *) The value to encode.
 *   (FILE *)      The output stream to write the given value to.
 */
extern void
 JsonEncodeValue(JsonValue * value, FILE * out);

/*
 * Encode a HashMap of JsonValues into a fully-valid, minimized JSON
 * object. This function is recursive; it will serialize everything
 * accessible from the passed object into JSON.
 *
 * Params:
 *
 *   (HashMap *) The HashMap of JsonValues to encode and write to the
 *               output stream.
 *   (FILE *)    The output stream to write the given HashMap to.
 *
 * Return: Whether or not the operation was successful. This function
 * will fail if either the passed HashMap or file stream are NULL. In
 * all other cases, this function succeeds.
 */
extern int
 JsonEncode(HashMap *, FILE *);

/*
 * Decode the given input stream into a HashMap of JsonValues.
 *
 * Params:
 *
 *   (FILE *) The input stream to parse JSON from.
 *
 * Return: A HashMap of JsonValues, or NULL if there was an error
 * parsing the JSON.
 */
extern HashMap *
 JsonDecode(FILE *);

#endif                             /* TELODENDRIA_JSON_H */
