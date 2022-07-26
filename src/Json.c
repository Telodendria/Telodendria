#include <Json.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

struct JsonValue
{
    JsonType type;
    union
    {
        HashMap *object;
        Array *array;
        char *string;
        int64_t integer;
        double floating;
        int boolean:1;
    } as;
};

JsonType
JsonValueType(JsonValue * value)
{
    if (!value)
    {
        return JSON_NULL;
    }

    return value->type;
}

static JsonValue *
JsonValueAllocate(void)
{
    return malloc(sizeof(JsonValue));
}

JsonValue *
JsonValueObject(HashMap * object)
{
    JsonValue *value;

    if (!object)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_OBJECT;
    value->as.object = object;

    return value;
}

HashMap *
JsonValueAsObject(JsonValue * value)
{
    if (!value || value->type != JSON_OBJECT)
    {
        return NULL;
    }

    return value->as.object;
}


JsonValue *
JsonValueArray(Array * array)
{
    JsonValue *value;

    if (!array)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_ARRAY;
    value->as.array = array;

    return value;
}

Array *
JsonValueAsArray(JsonValue * value)
{
    if (!value || value->type != JSON_ARRAY)
    {
        return NULL;
    }

    return value->as.array;
}


JsonValue *
JsonValueString(char *string)
{
    JsonValue *value;

    if (!string)
    {
        return NULL;
    }

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_STRING;
    value->as.string = string;

    return value;
}

char *
JsonValueAsString(JsonValue * value)
{
    if (!value || value->type != JSON_STRING)
    {
        return NULL;
    }

    return value->as.string;
}

JsonValue *
JsonValueInteger(int64_t integer)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return 0;
    }

    value->type = JSON_INTEGER;
    value->as.integer = integer;

    return value;
}

int64_t
JsonValueAsInteger(JsonValue * value)
{
    if (!value || value->type != JSON_INTEGER)
    {
        return 0;
    }

    return value->as.integer;
}


JsonValue *
JsonValueFloat(double floating)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_FLOAT;
    value->as.floating = floating;

    return value;
}

double
JsonValueAsFloat(JsonValue * value)
{
    if (!value || value->type != JSON_FLOAT)
    {
        return 0;
    }

    return value->as.floating;
}

JsonValue *
JsonValueBoolean(int boolean)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOLEAN;
    value->as.boolean = boolean;

    return value;
}

int
JsonValueAsBoolean(JsonValue * value)
{
    if (!value || value->type != JSON_BOOLEAN)
    {
        return 0;
    }

    return value->as.boolean;
}

JsonValue *
JsonValueNull(void)
{
    JsonValue *value;

    value = JsonValueAllocate();
    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NULL;

    return value;
}

void
JsonValueFree(JsonValue * value)
{
    if (!value)
    {
        return;
    }

    free(value);
}

static void
JsonEncodeValue(JsonValue * value, FILE * out)
{
    size_t i;
    size_t len;
    Array *arr;

    switch (value->type)
    {
        case JSON_OBJECT:
            JsonEncode(value->as.object, out);
            break;
        case JSON_ARRAY:
            arr = value->as.array;
            len = ArraySize(arr);

            fputc('[', out);

            for (i = 0; i < len; i++)
            {
                JsonEncodeValue(ArrayGet(arr, i), out);
                if (i < len - 1)
                {
                    fputc(',', out);
                }
            }

            fputc(']', out);
            break;
        case JSON_STRING:
            fprintf(out, "\"%s\"", value->as.string);
            break;
        case JSON_INTEGER:
            fprintf(out, "%lld", value->as.integer);
            break;
        case JSON_FLOAT:
            fprintf(out, "%f", value->as.floating);
            break;
        case JSON_BOOLEAN:
            if (value->as.boolean)
            {
                fputs("true", out);
            }
            else
            {
                fputs("false", out);
            }
            break;
        case JSON_NULL:
            fputs("null", out);
            break;
        default:
            return;
    }
}

int
JsonEncode(HashMap * object, FILE * out)
{
    size_t index;
    size_t count;
    char *key;
    JsonValue *value;

    if (!object || !out)
    {
        return 0;
    }

    count = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        count++;
    }

    fputc('{', out);

    index = 0;
    while (HashMapIterate(object, &key, (void **) &value))
    {
        fprintf(out, "\"%s\":", key);
        JsonEncodeValue(value, out);

        if (index < count - 1)
        {
            fputc(',', out);
        }

        index++;
    }

    fputc('}', out);

    return 1;
}
