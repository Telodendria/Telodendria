#ifndef TELODENDRIA_JSON_H
#define TELODENDRIA_JSON_H

#include <HashMap.h>
#include <Array.h>

#include <stdio.h>
#include <stddef.h>

typedef enum JsonType
{
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOAT,
    JSON_BOOLEAN,
    JSON_NULL
} JsonType;

typedef struct JsonValue JsonValue;

extern JsonType
 JsonValueType(JsonValue * value);

extern JsonValue *
 JsonValueObject(HashMap * object);

extern HashMap *
 JsonValueAsObject(JsonValue * value);

extern JsonValue *
 JsonValueArray(Array * array);

extern Array *
 JsonValueAsArray(JsonValue * value);

extern JsonValue *
 JsonValueString(char *string);

extern JsonValue *
 JsonValueInteger(long integer);

extern JsonValue *
 JsonValueFloat(double floating);

extern JsonValue *
 JsonValueBoolean(int boolean);

extern JsonValue *
 JsonValueNull(void);

extern void
 JsonValueFree(JsonValue * value);

extern void
 JsonFree(HashMap * object);

extern void
 JsonEncodeString(const char *str, FILE * out);

extern void
 JsonEncodeValue(JsonValue * value, FILE * out);

extern int
 JsonEncode(HashMap * object, FILE * out);

extern HashMap *
 JsonDecode(FILE * in);

#endif
