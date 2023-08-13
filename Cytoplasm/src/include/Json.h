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

#ifndef CYTOPLASM_JSON_H
#define CYTOPLASM_JSON_H

/***
 * @Nm Json
 * @Nd A fully-featured JSON API.
 * @Dd March 12 2023
 * @Xr HashMap Array Stream
 *
 * .Nm
 * is a fully-featured JSON API for C using the array and hash map
 * APIs. It can parse JSON, ans serialize an in-memory structure to
 * JSON. It build on the foundation of Array and HashMap because that's
 * all JSON really is, just arrays and maps.
 * .Nm
 * also provides a structure for encapsulating an arbitrary value and
 * identifying its type, making it easy for a strictly-typed language
 * like C to work with loosely-typed JSON data.
 * .Nm
 * is very strict and tries to adhere as closely as possible to the
 * proper definition of JSON. It will fail on syntax errors of any
 * kind, which is fine for a Matrix homeserver that can just return
 * M_BAD_JSON if anything in here fails, but this behavior may not be
 * suitable for other purposes.
 * .Pp
 * This JSON implementation focuses primarily on serialization and
 * deserialization to and from streams. It does not provide facilities
 * for handling JSON strings; it only writes JSON to output streams,
 * and reads them from input streams. Of course, you can use the
 * POSIX
 * .Xr fmemopen 3
 * and
 * .Xr open_memstream 3
 * functions if you want to deal with JSON strings, but JSON is
 * intended to be an exchange format. Data should be converted to JSON
 * right when it is leaving the program, and converted from JSON to the
 * in-memory format as soon as it is coming in.
 * .Pp
 * JSON objects are represented as hash maps consisting entirely of
 * JsonValue structures, and arrays are represented as arrays
 * consisting entirely of JsonValue structures. When generating a
 * JSON object, any attempt to stuff a value into a hash map or array
 * without first encoding it as a JsonValue will result in undefined
 * behavior.
 */

#include <HashMap.h>
#include <Array.h>
#include <Stream.h>
#include <Int64.h>

#include <stdio.h>
#include <stddef.h>

#define JSON_DEFAULT -1
#define JSON_PRETTY 0

/**
 * This opaque structure encapsulates all the possible types that can
 * be stored in JSON. It is managed entirely by the functions defined
 * in this API. It is important to note that strings, integers, floats,
 * booleans, and the NULL value are all stored by value, but objects
 * and arrays are stored by reference. That is, it doesn't store these
 * itself, just pointers to them, however, the data
 * .Em is
 * freed when using
 * .Fn JsonFree .
 */
typedef struct JsonValue JsonValue;

/**
 * These are the types that can be used to identify a JsonValue
 * and act on it accordingly.
 */
typedef enum JsonType
{
    JSON_NULL,    /* Maps to a C NULL */
    JSON_OBJECT,  /* Maps to a HashMap of JsonValues */
    JSON_ARRAY,   /* Maps to an Array of JsonValues */
    JSON_STRING,  /* Maps to a null-terminated C string */
    JSON_INTEGER, /* Maps to an Int64  */
    JSON_FLOAT,   /* Maps to a C double */
    JSON_BOOLEAN  /* Maps to a C integer of either 0 or 1 */
} JsonType;

/**
 * Determine the type of the specified JSON value.
 */
extern JsonType JsonValueType(JsonValue *);

/**
 * Encode a JSON object as a JSON value that can be added to another
 * object, or an array.
 */
extern JsonValue * JsonValueObject(HashMap *);

/**
 * Unwrap a JSON value that represents an object. This function will
 * return NULL if the value is not actually an object.
 */
extern HashMap * JsonValueAsObject(JsonValue *);

/**
 * Encode a JSON array as a JSON value that can be added to an object
 * or another array.
 */
extern JsonValue * JsonValueArray(Array *);

/**
 * Unwrap a JSON value that represents an array. This function will
 * return NULL if the value is not actually an array.
 */
extern Array * JsonValueAsArray(JsonValue *);

/**
 * Encode a C string as a JSON value that can be added to an object or
 * an array.
 */
extern JsonValue * JsonValueString(char *);

/**
 * Unwrap a JSON value that represents a string. This function will
 * return NULL if the value is not actually a string.
 */
extern char * JsonValueAsString(JsonValue *);

/**
 * Encode a number as a JSON value that can be added to an object or
 * an array.
 */
extern JsonValue * JsonValueInteger(Int64);

/**
 * Unwrap a JSON value that represents a number. This function will
 * return 0 if the value is not actually a number, which may be
 * misleading. Check the type of the value before making assumptions
 * about its value.
 */
extern Int64 JsonValueAsInteger(JsonValue *);

/**
 * Encode a floating point number as a JSON value that can be added
 * to an object or an array.
 */
extern JsonValue * JsonValueFloat(double);

/**
 * Unwrap a JSON value that represents a floating point number. This
 * function will return 0 if the value is not actually a floating
 * point number, which may be misleading. Check the type of the value
 * before making assumptions about its type.
 */
extern double JsonValueAsFloat(JsonValue *);

/**
 * Encode a C integer according to the way C treats integers in boolean
 * expressions as a JSON value that can be added to an object or an
 * array.
 */
extern JsonValue * JsonValueBoolean(int);

/**
 * Unwrap a JSON value that represents a boolean. This function will
 * return 0 if the value is not actually a boolean, which may be
 * misleading. Check the type of the value before making assumptions
 * about its type.
 */
extern int JsonValueAsBoolean(JsonValue *);

/**
 * This is a special case that represents a JSON null. Because the
 * Array and HashMap APIs do not accept NULL values, this function
 * should be used to represent NULL in JSON. Even though a small
 * amount of memory is allocated just to be a placeholder for nothing,
 * this keeps the APIs clean.
 */
extern JsonValue * JsonValueNull(void);

/**
 * Free the memory being used by a JSON value. Note that this will
 * recursively free all Arrays, HashMaps, and other JsonValues that are
 * reachable from the given value, including any strings attached to
 * this value.
 */
extern void JsonValueFree(JsonValue *);

/**
 * Recursively duplicate the given JSON value. This returns a new
 * JSON value that is completely identical to the specified value, but
 * in no way connected to it.
 */
extern JsonValue * JsonValueDuplicate(JsonValue *);

/**
 * Recursively duplicate the given JSON object. This returns a new
 * JSON object that is completely identical to the specified object,
 * but in no way connect to it.
 */
extern HashMap * JsonDuplicate(HashMap *);

/**
 * Recursively free a JSON object by iterating over all of its values
 * and freeing them using
 * .Fn JsonValueFree .
 */
extern void JsonFree(HashMap *);

/**
 * Encode the given string in such a way that it can be safely
 * embedded in a JSON stream. This entails:
 * .Bl -bullet -offset indent
 * .It
 * Escaping quotes, backslashes, and other special characters using
 * their backslash escape.
 * .It
 * Encoding bytes that are not UTF-8 using escapes.
 * .It
 * Wrapping the entire string in double quotes.
 * .El
 * .Pp
 * This function is only provided via the public
 * .Nm
 * API so that it is accessible to custom JSON encoders, such as the
 * CanonicalJson encoder. This will typically be used for encoding
 * object keys; to encode values, just use
 * .Fn JsonEncodeValue .
 * .Pp
 * This function returns the number of bytes written to the stream,
 * or if the stream is NULL, the number of bytes that would have
 * been written.
 */
extern int JsonEncodeString(const char *, Stream *);

/**
 * Serialize a JSON value as it would appear in JSON output. This is
 * a recursive function that also encodes all child values reachable
 * from the given value. This function is exposed via the public
 * .Nm
 * API so that it is accessible to custom JSON encoders. Normal users
 * that are not writing custom encoders should in most cases just use
 * .Fn JsonEncode
 * to encode an entire object.
 * .Pp
 * The third parameter is an integer that represents the indent level
 * of the value to be printed, or a negative number if pretty-printing
 * should be disabled and JSON should be printed as minimized as
 * possible. To pretty-print a JSON object, set this to
 * .Va JSON_PRETTY .
 * To get minified output, set it to
 * .Va JSON_DEFAULT .
 * .Pp
 * This function returns the number of bytes written to the stream,
 * or if the stream is NULL, the number of bytes that would have
 * been written.
 */
extern int JsonEncodeValue(JsonValue *, Stream *, int);

/**
 * Encode a JSON object as it would appear in JSON output, writing it
 * to the given output stream. This function is recursive; it will
 * serialize everything accessible from the passed object. The third
 * parameter has the same behavior as described above.
 * .Pp
 * This function returns the number of bytes written to the stream,
 * or if the stream is NULL, the number of bytes that would have
 * been written.
 */
extern int JsonEncode(HashMap *, Stream *, int);

/**
 * Decode a JSON object from the given input stream and parse it into
 * a hash map of JSON values.
 */
extern HashMap * JsonDecode(Stream *);

/**
 * A convenience function that allows the caller to retrieve and
 * arbitrarily deep keys within a JSON object. It takes a root JSON
 * object, the number of levels deep to go, and then that number of
 * keys as a varargs list. All keys must have objects as values, with
 * the exception of the last one, which is the value that will be
 * returned. Otherwise, NULL indicates the specified path doas not
 * exist.
 */
extern JsonValue * JsonGet(HashMap *, size_t,...);

/**
 * A convenience function that allows the caller to set arbitrarily
 * deep keys within a JSON object. It takes a root JSON object, the
 * number of levels deep to go, and then that number of keys as a
 * varargs list. All keys must have object as values, with the
 * exception of the last one, which is the value that will be set.
 * The value currently at that key, if any, will be returned.
 * This function will create any intermediate objects as necessary to
 * set the proper key.
 */
extern JsonValue * JsonSet(HashMap *, JsonValue *, size_t,...);

#endif                             /* CYTOPLASM_JSON_H */
