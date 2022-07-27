#include <Json.h>

#include <Util.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct JsonValue
{
    JsonType type;
    union
    {
        HashMap *object;
        Array *array;
        char *string;
        long integer;
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
JsonValueInteger(long integer)
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

long
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
JsonEncodeString(const char *str, FILE * out)
{
    size_t i;
    char c;

    fputc('"', out);

    i = 0;
    while ((c = str[i]) != '\0')
    {
        switch (c)
        {
            case '\\':
            case '"':
            case '/':
                fputc('\\', out);
                fputc(c, out);
                break;
            case '\b':
                fputs("\\b", out);
                break;
            case '\t':
                fputs("\\t", out);
                break;
            case '\n':
                fputs("\\n", out);
                break;
            case '\f':
                fputs("\\f", out);
                break;
            case '\r':
                fputs("\\r", out);
                break;
            default:              /* Assume UTF-8 input */
                fputc(c, out);
                break;

        }
        i++;
    }

    fputc('"', out);
}

static char *
JsonDecodeString(FILE * in)
{
    const size_t strBlockSize = 16;

    size_t i;
    size_t len;
    size_t allocated;
    char *str;
    char c;
    char a[5];

    unsigned long utf8;
    char *utf8Ptr;

    len = 0;
    allocated = strBlockSize;

    str = malloc(allocated * sizeof(char));
    if (!str)
    {
        return NULL;
    }

    while ((c = fgetc(in)) != EOF)
    {
        switch (c)
        {
            case '"':
                str[len] = '\0';
                return str;
                break;
            case '\\':
                c = fgetc(in);
                switch (c)
                {
                    case '\\':
                    case '"':
                    case '/':
                        a[0] = c;
                        a[1] = '\0';
                        break;
                    case 'b':
                        a[0] = '\b';
                        a[1] = '\0';
                        break;
                    case 't':
                        a[0] = '\t';
                        a[1] = '\0';
                        break;
                    case 'n':
                        a[0] = '\n';
                        a[1] = '\0';
                        break;
                    case 'f':
                        a[0] = '\f';
                        a[1] = '\0';
                        break;
                    case 'r':
                        a[0] = '\r';
                        a[1] = '\0';
                        break;
                    case 'u':
                        /* Read \uXXXX point into a 4-byte buffer */
                        if (fscanf(in, "%04lx", &utf8) != 1)
                        {
                            /* Bad hex value */
                            free(str);
                            return NULL;
                        }

                        /* Encode the 4-byte UTF-8 buffer into a series
                         * of 1-byte characters */
                        utf8Ptr = UtilUtf8Encode(utf8);
                        if (!utf8Ptr)
                        {
                            /* Mem error */
                            free(str);
                            return NULL;
                        }

                        /* Move the output of UtilUtf8Encode() into our
                         * local buffer */
                        strcpy(a, utf8Ptr);
                        free(utf8Ptr);
                        break;
                    default:
                        /* Bad escape value */
                        free(str);
                        return NULL;
                }
                break;
            default:
                a[0] = c;
                a[1] = '\0';
                break;
        }

        /* Append buffer a */
        i = 0;
        while (a[i] != '\0')
        {
            if (len >= allocated)
            {
                char *tmp;

                allocated += strBlockSize;
                tmp = realloc(str, allocated * sizeof(char));
                if (!tmp)
                {
                    free(str);
                    return NULL;
                }

                str = tmp;
            }

            str[len] = a[i];
            len++;
            i++;
        }
    }

    free(str);
    return NULL;
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
            JsonEncodeString(value->as.string, out);
            break;
        case JSON_INTEGER:
            fprintf(out, "%ld", value->as.integer);
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
        JsonEncodeString(key, out);
        fputc(':', out);
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
