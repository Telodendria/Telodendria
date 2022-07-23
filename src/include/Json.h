#ifndef TELODENDRIA_JSON_H
#define TELODENDRIA_JSON_H

#include <HashMap.h>
#include <Array.h>

typedef enum JsonType {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOAT,
    JSON_BOOLEAN,
    JSON_NULL
} JsonType;

typedef struct JsonValue {
    JsonType type;
    union as {
        HashMap *object;
        Array *array;
        char *string;
        int64_t integer;
        double floating;
        int boolean : 1;
    };
} JsonValue;


extern JsonType
JsonValueType(JsonValue *value);

extern JsonValue *
JsonValueObject(HashMap *object);

extern HashMap *
JsonValueAsObject(JsonValue *value);

extern JsonValue *
JsonValueArray(Array *array);

extern Array *
JsonValueAsArray(JsonValue *value);

extern JsonValue *
JsonValueString(char *string);

extern JsonValue *
JsonValueInteger(int64_t integer);

extern JsonValue *
JsonValueFloat(double floating);

extern JsonValue *
JsonValueBoolean(int boolean);

extern JsonValue *
JsonValueNull(void);

extern void *
JsonValueFree(JsonValue *value);

extern char *
JsonEncode(HashMap *object);

extern HashMap *
JsonDecode(char *string);

#endif
