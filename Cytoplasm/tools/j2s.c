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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Log.h>
#include <Array.h>
#include <Graph.h>
#include <HashMap.h>
#include <Args.h>
#include <Json.h>
#include <Str.h>
#include <Memory.h>

#define MAX_DEPENDENCIES 32

static char *
Trim(char c, char *str)
{
    while (*str == c)
        str++;
    return str;
}

static JsonType
TypeToJsonType(char *type)
{
    if (StrEquals(type, "object"))
    {
        return JSON_OBJECT;
    }
    else if (StrEquals(type, "array"))
    {
        return JSON_ARRAY;
    }
    else if (StrEquals(type, "string"))
    {
        return JSON_STRING;
    }
    else if (StrEquals(type, "integer"))
    {
        return JSON_INTEGER;
    }
    else if (StrEquals(type, "float"))
    {
        return JSON_FLOAT;
    }
    else if (StrEquals(type, "boolean"))
    {
        return JSON_BOOLEAN;
    }
    else
    {
        if (*type == '[' && type[strlen(type) - 1] == ']')
        {
            return JSON_ARRAY;
        }
        else
        {
            return JSON_OBJECT;
        }
    }
}

static char *
JsonTypeToStr(JsonType type)
{
    switch (type)
    {
            case JSON_OBJECT:
            return "JSON_OBJECT";
        case JSON_ARRAY:
            return "JSON_ARRAY";
        case JSON_STRING:
            return "JSON_STRING";
        case JSON_INTEGER:
            return "JSON_INTEGER";
        case JSON_FLOAT:
            return "JSON_FLOAT";
        case JSON_BOOLEAN:
            return "JSON_BOOLEAN";
        case JSON_NULL:
        default:
            return "JSON_NULL";
    }
}

int
Main(Array * args)
{
    ArgParseState arg;
    int opt;
    int ret = EXIT_FAILURE;

    char *schema = NULL;
    char *header = NULL;
    char *impl = NULL;

    Stream *schemaFile = NULL;
    Stream *headerFile = NULL;
    Stream *implFile = NULL;

    HashMap *schemaJson = NULL;

    JsonValue *val;
    char *type;
    JsonValue *typeVal;

    HashMap *types;

    size_t i;

    Graph *dependencyGraph = GraphCreate(MAX_DEPENDENCIES);
    HashMap *typeToNode = HashMapCreate();
    Array *requiredTypes = ArrayCreate();

    Node *sortedNodes;
    size_t sortedNodesLen;
    Node *node;

    char *guard;
    char *headerName;

    if (!requiredTypes)
    {
        Log(LOG_ERR, "Unable to allocate memory for type name storage");
        goto finish;
    }

    ArgParseStateInit(&arg);
    while ((opt = ArgParse(&arg, args, "s:h:c:")) != -1)
    {
        switch (opt)
        {
            case 's':
                schema = arg.optArg;
                break;
            case 'h':
                header = arg.optArg;
                break;
            case 'c':
                impl = arg.optArg;
                break;
            default:
                goto finish;
        }
    }

    if (!schema)
    {
        Log(LOG_ERR, "Please specify a schema with -s");
        goto finish;
    }

    if (!header)
    {
        Log(LOG_ERR, "Please specify an output header with -h");
        goto finish;
    }

    if (!impl)
    {
        Log(LOG_ERR, "Please specify an output C source file with -c");
        goto finish;
    }

    schemaFile = StreamOpen(schema, "r");
    if (!schemaFile)
    {
        Log(LOG_ERR, "Unable to open '%s' for reading.", schema);
        goto finish;
    }

    headerFile = StreamOpen(header, "w");
    if (!headerFile)
    {
        Log(LOG_ERR, "Unable to open '%s' for writing.", header);
        goto finish;
    }

    implFile = StreamOpen(impl, "w");
    if (!implFile)
    {
        Log(LOG_ERR, "Unable to open '%s' for writing.", impl);
        goto finish;
    }

    schemaJson = JsonDecode(schemaFile);
    if (!schemaJson)
    {
        Log(LOG_ERR, "JSON syntax error.");
        goto finish;
    }

    StreamClose(schemaFile);
    schemaFile = NULL;

    val = HashMapGet(schemaJson, "guard");

    if (!val)
    {
        Log(LOG_WARNING, "Guard not specified, using 'J2S_SCHEMA_H', which may not be unique.");
        HashMapSet(schemaJson, "guard", JsonValueString("J2S_SCHEMA_H"));
    }
    else if (JsonValueType(val) != JSON_STRING)
    {
        Log(LOG_ERR, "Validation error: 'guard' must be a string.");
        goto finish;
    }

    val = HashMapGet(schemaJson, "header");
    if (!val)
    {
        Log(LOG_WARNING, "Header name not specified, using 'Schema.h', which is probably wrong.");
        HashMapSet(schemaJson, "header", JsonValueString("Schema.h"));
    }
    else if (JsonValueType(val) != JSON_STRING)
    {
        Log(LOG_ERR, "Validation error: 'header' must be a string.");
        goto finish;
    }

    val = HashMapGet(schemaJson, "includes");

    if (val && JsonValueType(val) != JSON_ARRAY)
    {
        Log(LOG_ERR, "Validation error: 'includes' must be an array.");
        goto finish;
    }

    for (i = 0; i < ArraySize(JsonValueAsArray(val)); i++)
    {
        if (JsonValueType(ArrayGet(JsonValueAsArray(val), 0)) != JSON_STRING)
        {
            Log(LOG_ERR, "Validation error: 'includes[%lu]' is not a string.", i);
            goto finish;
        }
    }

    val = HashMapGet(schemaJson, "types");
    if (JsonValueType(val) != JSON_OBJECT)
    {
        Log(LOG_ERR, "Validation error: 'types' must be an object.");
        goto finish;
    }

    types = JsonValueAsObject(val);

    while (HashMapIterate(types, &type, (void **) &typeVal))
    {
        HashMap *typeObj;
        JsonValue *typeTypeVal;
        char *typeType;

        JsonValue *typeFieldsVal;
        HashMap *typeFields;

        char *fieldName;
        JsonValue *fieldVal;
        HashMap *fieldObj;

        Node *typeNode;

        if (JsonValueType(typeVal) != JSON_OBJECT)
        {
            Log(LOG_ERR, "Validation error: 'types.%s' must be an object.", type);
            goto finish;
        }

        typeObj = JsonValueAsObject(typeVal);
        typeTypeVal = HashMapGet(typeObj, "type");
        if (JsonValueType(typeTypeVal) != JSON_STRING)
        {
            Log(LOG_ERR, "Validation error: 'types.%s.type' must be a string.", type);
            goto finish;
        }

        typeType = JsonValueAsString(typeTypeVal);

        typeNode = HashMapGet(typeToNode, type);
        if (!typeNode)
        {
            typeNode = Malloc(sizeof(Node));
            *typeNode = ArraySize(requiredTypes);
            HashMapSet(typeToNode, type, typeNode);
            ArrayAdd(requiredTypes, StrDuplicate(type));
        }

        typeFieldsVal = HashMapGet(typeObj, "fields");
        if (JsonValueType(typeFieldsVal) != JSON_OBJECT)
        {
            Log(LOG_ERR, "Validation error: 'types.%s.fields' must be an object.", type);
            goto finish;
        }

        typeFields = JsonValueAsObject(typeFieldsVal);

        if (StrEquals(typeType, "struct"))
        {
            while (HashMapIterate(typeFields, &fieldName, (void **) &fieldVal))
            {
                char *fieldType;
                int isArrType = 0;
                JsonValue *requiredVal;

                if (JsonValueType(fieldVal) != JSON_OBJECT)
                {
                    Log(LOG_ERR, "Validation error: 'types.%s.fields.%s' must be an object.", type, fieldName);
                    goto finish;
                }

                fieldObj = JsonValueAsObject(fieldVal);

                fieldType = JsonValueAsString(HashMapGet(fieldObj, "type"));
                if (!fieldType)
                {
                    Log(LOG_ERR, "Validation error: 'types.%s.fields.%s.type' is required and must be a string.", type, fieldName);
                    goto finish;
                }

                if (*fieldType == '[' && fieldType[strlen(fieldType) - 1] == ']')
                {
                    fieldType++;
                    fieldType[strlen(fieldType) - 1] = '\0';
                    isArrType = 1;
                }

                if (!StrEquals(fieldType, "object") &&
                    !StrEquals(fieldType, "array") &&
                    !StrEquals(fieldType, "string") &&
                    !StrEquals(fieldType, "integer") &&
                    !StrEquals(fieldType, "float") &&
                    !StrEquals(fieldType, "boolean"))
                {
                    Node *node = HashMapGet(typeToNode, fieldType);

                    if (!node)
                    {
                        node = Malloc(sizeof(Node));
                        *node = ArraySize(requiredTypes);
                        HashMapSet(typeToNode, fieldType, node);
                        ArrayAdd(requiredTypes, StrDuplicate(fieldType));
                    }

                    GraphEdgeSet(dependencyGraph, *node, *typeNode, 1);
                }

                if (isArrType)
                {
                    fieldType[strlen(fieldType)] = ']';
                }

                requiredVal = HashMapGet(fieldObj, "required");
                if (requiredVal && JsonValueType(requiredVal) != JSON_BOOLEAN)
                {
                    Log(LOG_ERR, "Validation error: 'types.%s.fields.%s.required' must be a boolean.", type, fieldName);
                    goto finish;
                }
            }
        }
        else if (StrEquals(typeType, "enum"))
        {
            while (HashMapIterate(typeFields, &fieldName, (void **) &fieldVal))
            {
                char *name;

                if (JsonValueType(fieldVal) != JSON_OBJECT)
                {
                    Log(LOG_ERR, "Validation error: 'types.%s.fields.%s' must be an object.", type, fieldName);
                    goto finish;
                }

                fieldObj = JsonValueAsObject(fieldVal);

                name = JsonValueAsString(HashMapGet(fieldObj, "name"));
                if (!name)
                {
                    Log(LOG_ERR, "Validation error: 'types.%s.fields.%s.name' is required and must be a string.", type, fieldName);
                    goto finish;
                }
            }
        }
        else
        {
            Log(LOG_ERR, "Validation error: 'types.%s.type' must be 'struct' or 'enum'.", type);
            goto finish;
        }
        /*
         * TODO: Add "extern" type that doesn't actually generate any code,
         * but trusts the user that it has been generated somewhere else. This
         * is effectively "importing" types.
         */
    }

    sortedNodes = GraphTopologicalSort(dependencyGraph, &sortedNodesLen);

    GraphFree(dependencyGraph);

    for (i = 0; i < sortedNodesLen; i++)
    {
        char *type = ArrayGet(requiredTypes, sortedNodes[i]);

        if (!type)
        {
            continue;
        }

        if (!HashMapGet(types, type))
        {
            Log(LOG_ERR, "No type definition for required type '%s'", type);
            goto finish;
        }
    }

    guard = JsonValueAsString(HashMapGet(schemaJson, "guard"));

    StreamPrintf(headerFile, "/* Generated by j2s */\n\n");
    StreamPrintf(headerFile, "#ifndef %s\n", guard);
    StreamPrintf(headerFile, "#define %s\n\n", guard);

    StreamPrintf(headerFile, "#include <Array.h>\n");
    StreamPrintf(headerFile, "#include <HashMap.h>\n");
    StreamPrintf(headerFile, "#include <Int64.h>\n");

    StreamPutc(headerFile, '\n');

    for (i = 0; i < ArraySize(JsonValueAsArray(HashMapGet(schemaJson, "include"))); i++)
    {
        char *h = JsonValueAsString(ArrayGet(JsonValueAsArray(HashMapGet(schemaJson, "include")), i));

        StreamPrintf(headerFile, "#include <%s>\n", h);
    }

    StreamPutc(headerFile, '\n');

    for (i = 0; i < sortedNodesLen; i++)
    {
        char *type = ArrayGet(requiredTypes, sortedNodes[i]);
        char *typeType;
        HashMap *fields;

        char *field;
        HashMap *fieldDesc;

        if (!type)
        {
            continue;
        }

        typeType = JsonValueAsString(JsonGet(types, 2, type, "type"));
        fields = JsonValueAsObject(JsonGet(types, 2, type, "fields"));

        StreamPrintf(headerFile, "typedef %s %s\n{\n", typeType, type);

        if (StrEquals(typeType, "struct"))
        {
            while (HashMapIterate(fields, &field, (void **) &fieldDesc))
            {
                char *fieldType;
                char *cType;

                fieldDesc = JsonValueAsObject((JsonValue *) fieldDesc);

                fieldType = JsonValueAsString(HashMapGet(fieldDesc, "type"));

                if (StrEquals(fieldType, "string"))
                {
                    cType = "char *";
                }
                else if (StrEquals(fieldType, "integer"))
                {
                    cType = "Int64";
                }
                else if (StrEquals(fieldType, "boolean"))
                {
                    cType = "int";
                }
                else if (StrEquals(fieldType, "float"))
                {
                    cType = "double";
                }
                else if (StrEquals(fieldType, "object"))
                {
                    cType = "HashMap *";
                }
                else if (StrEquals(fieldType, "array") || (*fieldType == '[' && fieldType[strlen(fieldType) - 1] == ']'))
                {
                    cType = "Array *";
                }
                else
                {
                    cType = fieldType;
                }

                StreamPrintf(headerFile, "    %s %s;\n", cType, field);
            }

            StreamPrintf(headerFile, "} %s;\n\n", type);
        }
        else if (StrEquals(typeType, "enum"))
        {
            /*
             * Enums must be copied to an array because we have to know
             * where we're at to know whether or not we need the trailing
             * comma.
             */
            Array *keys = HashMapKeys(fields);

            size_t j;

            for (j = 0; j < ArraySize(keys); j++)
            {
                char *key = ArrayGet(keys, j);
                char *name;
                char *value;

                fieldDesc = JsonValueAsObject(HashMapGet(fields, key));

                name = JsonValueAsString(HashMapGet(fieldDesc, "name"));
                value = JsonValueAsString(HashMapGet(fieldDesc, "value"));

                if (value)
                {
                    StreamPrintf(headerFile, "    %s = %s", name, value);
                }
                else
                {
                    StreamPrintf(headerFile, "    %s", name);
                }

                if (j < ArraySize(keys) - 1)
                {
                    StreamPutc(headerFile, ',');
                }

                StreamPutc(headerFile, '\n');
            }

            ArrayFree(keys);

            StreamPrintf(headerFile, "} %s;\n\n", type);
        }
    }

    Free(sortedNodes);
    for (i = 0; i < ArraySize(requiredTypes); i++)
    {
        Free(ArrayGet(requiredTypes, i));
    }
    ArrayFree(requiredTypes);

    while (HashMapIterate(typeToNode, &type, (void **) &node))
    {
        Free(node);
    }
    HashMapFree(typeToNode);

    headerName = JsonValueAsString(HashMapGet(schemaJson, "header"));

    StreamPrintf(implFile, "/* Generated by j2s */\n\n");
    StreamPrintf(implFile, "#include <%s>\n\n", headerName);

    StreamPrintf(implFile, "#include <Memory.h>\n");
    StreamPrintf(implFile, "#include <Json.h>\n");
    StreamPrintf(implFile, "#include <Str.h>\n");

    StreamPutc(implFile, '\n');

    while (HashMapIterate(types, &type, (void **) &typeVal))
    {
        char *typeType = JsonValueAsString(JsonGet(types, 2, type, "type"));
        HashMap *fields = JsonValueAsObject(JsonGet(types, 2, type, "fields"));

        Array *keys = HashMapKeys(fields);

        if (StrEquals(typeType, "struct"))
        {
            StreamPrintf(headerFile, "extern int %sFromJson(HashMap *, %s *, char **);\n", type, type);
            StreamPrintf(implFile, "int\n%sFromJson(HashMap *json, %s *out, char **errp)\n{\n", type, type);
            StreamPrintf(implFile, "    JsonValue *val;\n");
            StreamPrintf(implFile, "    int enumParseRes;\n");
            StreamPrintf(implFile, "\n");
            StreamPrintf(implFile, "    (void) enumParseRes;\n");
            StreamPrintf(implFile, "\n");
            StreamPrintf(implFile, "    if (!json | !out)\n"
                         "    {\n"
                         "        *errp = \"Invalid pointers passed to %sFromJson()\";\n"
                         "        return 0;\n"
                         "    }\n\n"
                         ,type);
            for (i = 0; i < ArraySize(keys); i++)
            {
                char *key = ArrayGet(keys, i);
                int required = JsonValueAsBoolean(JsonGet(fields, 2, key, "required"));
                char *fieldType = JsonValueAsString(JsonGet(fields, 2, key, "type"));
                int isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");
                JsonType jsonType = isEnum ? JSON_STRING : TypeToJsonType(fieldType);
                char *jsonTypeStr = JsonTypeToStr(jsonType);

                StreamPrintf(implFile, "    val = HashMapGet(json, \"%s\");\n", Trim('_', key));

                StreamPrintf(implFile, "    if (val)\n    {\n");
                StreamPrintf(implFile, "        if (JsonValueType(val) != %s)\n        {\n", jsonTypeStr);
                StreamPrintf(implFile, "            *errp = \"%s.%s must be of type %s.\";\n", type, Trim('_', key), fieldType);
                StreamPrintf(implFile, "            return 0;\n");
                StreamPrintf(implFile, "        }\n\n");
                if (StrEquals(fieldType, "array"))
                {
                    StreamPrintf(implFile, "        out->%s = JsonValueAsArray(JsonValueDuplicate(val));\n", key);
                }
                else if (StrEquals(fieldType, "object"))
                {
                    StreamPrintf(implFile, "        out->%s = JsonValueAsObject(JsonValueDuplicate(val));\n", key);
                }
                else if (*fieldType == '[' && fieldType[strlen(fieldType) - 1] == ']')
                {
                    fieldType++;
                    fieldType[strlen(fieldType) - 1] = '\0';
                    isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");
                    jsonType = isEnum ? JSON_STRING : TypeToJsonType(fieldType);

                    StreamPrintf(implFile, "        out->%s = ArrayCreate();\n", key);
                    StreamPrintf(implFile, "        if (!out->%s)\n", key);
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            *errp = \"Failed to allocate memory for %s.%s.\";\n", type, key);
                    StreamPrintf(implFile, "            return 0;\n");
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        else\n");
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            Array *arr = JsonValueAsArray(val);\n");
                    StreamPrintf(implFile, "            size_t i;\n");
                    StreamPrintf(implFile, "\n");
                    StreamPrintf(implFile, "            for (i = 0; i <ArraySize(arr); i++)\n");
                    StreamPrintf(implFile, "            {\n");
                    StreamPrintf(implFile, "                JsonValue *v = ArrayGet(arr, i);\n");

                    if (StrEquals(fieldType, "integer") ||
                        StrEquals(fieldType, "float") ||
                        StrEquals(fieldType, "boolean"))
                    {
                        char *cType;

                        if (StrEquals(fieldType, "integer"))
                        {
                            cType = "Int64";
                        }
                        else if (StrEquals(fieldType, "float"))
                        {
                            cType = "double";
                        }
                        else if (StrEquals(fieldType, "boolean"))
                        {
                            cType = "int";
                        }
                        else
                        {
                            /* Should never happen */
                            cType = NULL;
                        }

                        *fieldType = toupper(*fieldType);

                        StreamPrintf(implFile, "                %s *ref;\n", cType);
                        StreamPrintf(implFile, "                if (JsonValueType(v) != %s)\n", JsonTypeToStr(jsonType));
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"%s.%s[] contains an invalid value.\";\n", type, key);
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                ref = Malloc(sizeof(%s));\n", cType);
                        StreamPrintf(implFile, "                if (!ref)\n");
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"Unable to allocate memory for array value.\";\n");
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                *ref = JsonValueAs%s(v);\n", fieldType);
                        StreamPrintf(implFile, "                ArrayAdd(out->%s, ref);\n", key);

                        *fieldType = tolower(*fieldType);
                    }
                    else if (StrEquals(fieldType, "string"))
                    {
                        StreamPrintf(implFile, "                if (JsonValueType(v) != %s)\n", JsonTypeToStr(jsonType));
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"%s.%s[] contains an invalid value.\";\n", type, key);
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                ArrayAdd(out->%s, StrDuplicate(JsonValueAsString(v)));\n", key);
                    }
                    else if (StrEquals(fieldType, "object"))
                    {
                        StreamPrintf(implFile, "                if (JsonValueType(v) != %s)\n", JsonTypeToStr(jsonType));
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"%s.%s[] contains an invalid value.\";\n", type, key);
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                ArrayAdd(out->%s, JsonDuplicate(JsonValueAsObject(v)));\n", key);
                    }
                    else
                    {
                        if (isEnum)
                        {
                            StreamPrintf(implFile, "                int parseResult;\n");
                        }
                        StreamPrintf(implFile, "                %s *parsed;\n", fieldType);
                        StreamPrintf(implFile, "                if (JsonValueType(v) != %s)\n", JsonTypeToStr(jsonType));
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"%s.%s[] contains an invalid value.\";\n", type, key);
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                parsed = Malloc(sizeof(%s));\n", fieldType);
                        StreamPrintf(implFile, "                if (!parsed)\n");
                        StreamPrintf(implFile, "                {\n");
                        StreamPrintf(implFile, "                    *errp = \"Unable to allocate memory for array value.\";\n");
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        if (isEnum)
                        {
                            StreamPrintf(implFile, "                parseResult = %sFromStr(JsonValueAsString(v));\n", fieldType);
                            StreamPrintf(implFile, "                *parsed = parseResult;\n");
                            StreamPrintf(implFile, "                if (parseResult == -1)\n");
                        }
                        else
                        {
                            StreamPrintf(implFile, "                if (!%sFromJson(JsonValueAsObject(v), parsed, errp))\n", fieldType);
                        }
                        StreamPrintf(implFile, "                {\n");
                        if (!isEnum)
                        {
                            StreamPrintf(implFile, "                    %sFree(parsed);\n", fieldType);
                        }
                        StreamPrintf(implFile, "                    Free(parsed);\n");
                        StreamPrintf(implFile, "                    return 0;\n");
                        StreamPrintf(implFile, "                }\n");
                        StreamPrintf(implFile, "                ArrayAdd(out->%s, parsed);\n", key);
                    }

                    StreamPrintf(implFile, "            }\n");
                    StreamPrintf(implFile, "        }\n");

                    fieldType[strlen(fieldType)] = ']';
                }
                else if (jsonType == JSON_OBJECT)
                {
                    StreamPrintf(implFile, "        if (!%sFromJson(JsonValueAsObject(val), &out->%s, errp))\n        {\n", fieldType, key);
                    StreamPrintf(implFile, "            return 0;\n");
                    StreamPrintf(implFile, "        }\n");
                }
                else
                {
                    char *func;

                    switch (jsonType)
                    {
                        case JSON_STRING:
                            func = "String";
                            break;
                        case JSON_INTEGER:
                            func = "Integer";
                            break;
                        case JSON_FLOAT:
                            func = "Float";
                            break;
                        case JSON_BOOLEAN:
                            func = "Boolean";
                            break;
                        default:
                            /* Should not happen */
                            func = NULL;
                            break;
                    }

                    if (jsonType == JSON_STRING)
                    {
                        if (isEnum)
                        {
                            StreamPrintf(implFile, "        enumParseRes = %sFromStr(JsonValueAs%s(val));\n", fieldType, func);
                            StreamPrintf(implFile, "        if (enumParseRes == -1)\n", key);
                            StreamPrintf(implFile, "        {\n");
                            StreamPrintf(implFile, "            *errp = \"Invalid value for %s.%s.\";\n", type, key);
                            StreamPrintf(implFile, "            return 0;\n");
                            StreamPrintf(implFile, "        }\n");
                            StreamPrintf(implFile, "        out->%s = enumParseRes;\n", key);
                        }
                        else
                        {
                            StreamPrintf(implFile, "        out->%s = StrDuplicate(JsonValueAs%s(val));\n", key, func);
                        }
                    }
                    else
                    {
                        StreamPrintf(implFile, "        out->%s = JsonValueAs%s(val);\n", key, func);
                    }
                }
                StreamPrintf(implFile, "    }\n");

                if (required)
                {
                    StreamPrintf(implFile, "    else\n    {\n");
                    StreamPrintf(implFile, "        *errp = \"%s.%s is required.\";\n", type, key);
                    StreamPrintf(implFile, "        return 0;\n");
                    StreamPrintf(implFile, "    }\n");
                }

                StreamPutc(implFile, '\n');
            }
            StreamPrintf(implFile, "    return 1;\n");
            StreamPrintf(implFile, "}\n\n");

            StreamPrintf(headerFile, "extern HashMap * %sToJson(%s *);\n", type, type);
            StreamPrintf(implFile, "HashMap *\n%sToJson(%s *val)\n{\n", type, type);
            StreamPrintf(implFile, "    HashMap *json;\n");
            StreamPrintf(implFile, "\n");
            StreamPrintf(implFile, "    if (!val)\n");
            StreamPrintf(implFile, "    {\n");
            StreamPrintf(implFile, "        return NULL;\n");
            StreamPrintf(implFile, "    }\n\n");
            StreamPrintf(implFile, "    json = HashMapCreate();\n");
            StreamPrintf(implFile, "    if (!json)\n");
            StreamPrintf(implFile, "    {\n");
            StreamPrintf(implFile, "        return NULL;\n");
            StreamPrintf(implFile, "    }\n\n");
            for (i = 0; i < ArraySize(keys); i++)
            {
                char *key = ArrayGet(keys, i);
                char *fieldType = JsonValueAsString(JsonGet(fields, 2, key, "type"));
                int isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");

                if (StrEquals(fieldType, "array"))
                {
                    StreamPrintf(implFile, "    if (val->%s)\n", key);
                    StreamPrintf(implFile, "    {\n");
                    StreamPrintf(implFile, "        size_t i;\n");
                    StreamPrintf(implFile, "        Array *jsonArr = ArrayCreate();\n");
                    StreamPrintf(implFile, "        if (!jsonArr)\n");
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            JsonFree(json);\n");
                    StreamPrintf(implFile, "            return NULL;\n");
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        for (i = 0; i < ArraySize(val->%s); i++)\n", key);
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            ArrayAdd(jsonArr, JsonValueDuplicate(ArrayGet(val->%s, i)));\n", key);
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        HashMapSet(json, \"%s\", JsonValueArray(jsonArr));\n", Trim('_', key));
                    StreamPrintf(implFile, "    }\n");
                }
                else if (StrEquals(fieldType, "object"))
                {
                    StreamPrintf(implFile, "    HashMapSet(json, \"%s\", JsonValueObject(JsonDuplicate(val->%s)));\n", Trim('_', key), key);
                }
                else if (*fieldType == '[' && fieldType[strlen(fieldType) - 1] == ']')
                {
                    int isPrimitive;

                    fieldType++;
                    fieldType[strlen(fieldType) - 1] = '\0';
                    isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");
                    isPrimitive = StrEquals(fieldType, "integer") ||
                            StrEquals(fieldType, "boolean") ||
                            StrEquals(fieldType, "float");


                    StreamPrintf(implFile, "    if (val->%s)\n", key);
                    StreamPrintf(implFile, "    {\n");
                    StreamPrintf(implFile, "        size_t i;\n");
                    StreamPrintf(implFile, "        Array *jsonArr = ArrayCreate();\n");
                    StreamPrintf(implFile, "        if (!jsonArr)\n");
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            JsonFree(json);\n");
                    StreamPrintf(implFile, "            return NULL;\n");
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        for (i = 0; i < ArraySize(val->%s); i++)\n", key);
                    StreamPrintf(implFile, "        {\n");

                    if (StrEquals(fieldType, "string"))
                    {
                        StreamPrintf(implFile, "            ArrayAdd(jsonArr, JsonValueString(ArrayGet(val->%s, i)));\n", key);
                    }
                    else if (!isPrimitive)
                    {
                        StreamPrintf(implFile, "            ArrayAdd(jsonArr, JsonValueObject(%sToJson(ArrayGet(val->%s, i))));\n", fieldType, key);
                    }
                    else
                    {
                        char *cType;

                        if (StrEquals(fieldType, "integer"))
                        {
                            cType = "Int64";
                        }
                        else if (StrEquals(fieldType, "float"))
                        {
                            cType = "double";
                        }
                        else if (StrEquals(fieldType, "boolean"))
                        {
                            cType = "int";
                        }
                        else
                        {
                            /* Should never happen */
                            cType = NULL;
                        }

                        *fieldType = toupper(*fieldType);
                        StreamPrintf(implFile, "            ArrayAdd(jsonArr, (JsonValue%s(*((%s *) ArrayGet(val->%s, i)))));\n", fieldType, cType, key);
                        *fieldType = tolower(*fieldType);
                    }

                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        HashMapSet(json, \"%s\", JsonValueArray(jsonArr));\n", Trim('_', key));
                    StreamPrintf(implFile, "    }\n");

                    fieldType[strlen(fieldType)] = ']';
                }
                else
                {
                    if (HashMapGet(types, fieldType))
                    {
                        if (isEnum)
                        {
                            StreamPrintf(implFile, "    HashMapSet(json, \"%s\", JsonValueString(%sToStr(val->%s)));\n", Trim('_', key), fieldType, key);
                        }
                        else
                        {
                            StreamPrintf(implFile, "    HashMapSet(json, \"%s\", JsonValueObject(%sToJson(&val->%s)));\n", Trim('_', key), fieldType, key);
                        }
                    }
                    else
                    {
                        *fieldType = toupper(*fieldType);
                        StreamPrintf(implFile, "    HashMapSet(json, \"%s\", JsonValue%s(val->%s));\n", Trim('_', key), fieldType, key);
                        *fieldType = tolower(*fieldType);
                    }
                }
            }
            StreamPrintf(implFile, "    return json;\n");
            StreamPrintf(implFile, "}\n\n");

            StreamPrintf(headerFile, "extern void %sFree(%s *);\n", type, type);
            StreamPrintf(implFile, "void\n%sFree(%s *val)\n{\n", type, type);
            StreamPrintf(implFile, "    if (!val)\n");
            StreamPrintf(implFile, "    {\n");
            StreamPrintf(implFile, "        return;\n");
            StreamPrintf(implFile, "    }\n\n");
            for (i = 0; i < ArraySize(keys); i++)
            {
                char *key = ArrayGet(keys, i);
                char *fieldType = JsonValueAsString(JsonGet(fields, 2, key, "type"));
                int isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");

                if (StrEquals(fieldType, "array"))
                {
                    StreamPrintf(implFile, "    if (val->%s)\n", key);
                    StreamPrintf(implFile, "    {\n");
                    StreamPrintf(implFile, "        size_t i;\n");
                    StreamPrintf(implFile, "        for (i = 0; i < ArraySize(val->%s); i++)\n", key);
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            JsonValueFree(ArrayGet(val->%s, i));\n", key);
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        ArrayFree(val->%s);\n", key);
                    StreamPrintf(implFile, "    }\n");
                }
                else if (StrEquals(fieldType, "object"))
                {
                    StreamPrintf(implFile, "    JsonFree(val->%s);\n", key);
                }
                else if (StrEquals(fieldType, "string"))
                {
                    StreamPrintf(implFile, "    Free(val->%s);\n", key);
                }
                else if (*fieldType == '[' && fieldType[strlen(fieldType) - 1] == ']')
                {
                    int isPrimitive;

                    fieldType++;
                    fieldType[strlen(fieldType) - 1] = '\0';
                    isEnum = StrEquals(JsonValueAsString(JsonGet(types, 2, fieldType, "type")), "enum");
                    isPrimitive = StrEquals(fieldType, "boolean") ||
                            StrEquals(fieldType, "float") ||
                            StrEquals(fieldType, "integer") ||
                            StrEquals(fieldType, "string");

                    StreamPrintf(implFile, "    if (val->%s)\n", key);
                    StreamPrintf(implFile, "    {\n");
                    StreamPrintf(implFile, "        size_t i;\n");
                    StreamPrintf(implFile, "        for (i = 0; i < ArraySize(val->%s); i++)\n", key);
                    StreamPrintf(implFile, "        {\n");
                    StreamPrintf(implFile, "            %sFree(ArrayGet(val->%s, i));\n", (!isEnum && !isPrimitive) ? fieldType : "", key);
                    StreamPrintf(implFile, "        }\n");
                    StreamPrintf(implFile, "        ArrayFree(val->%s);\n", key);
                    StreamPrintf(implFile, "    }\n");

                    fieldType[strlen(fieldType)] = ']';
                }
                else
                {
                    /* Ignore primitives but call the appropriate free
                     * method on declared types */
                    if (!isEnum && HashMapGet(types, fieldType))
                    {
                        StreamPrintf(implFile, "    %sFree(&val->%s);\n", fieldType, key);
                    }
                }
            }
            StreamPrintf(implFile, "}\n\n");
            StreamPutc(headerFile, '\n');
        }
        else if (StrEquals(typeType, "enum"))
        {
            StreamPrintf(headerFile, "extern int %sFromStr(char *);\n", type);
            StreamPrintf(implFile, "int\n%sFromStr(char *str)\n{\n", type);
            for (i = 0; i < ArraySize(keys); i++)
            {
                char *key = ArrayGet(keys, i);

                if (i > 0)
                {
                    StreamPrintf(implFile, "    else if");
                }
                else
                {
                    StreamPrintf(implFile, "    if");
                }

                StreamPrintf(implFile, " (StrEquals(str, \"%s\"))\n    {\n", Trim('_', key));
                StreamPrintf(implFile, "        return %s;\n", JsonValueAsString(JsonGet(fields, 2, key, "name")));
                StreamPrintf(implFile, "    }\n");
            }

            StreamPrintf(implFile, "    else\n    {\n");
            StreamPrintf(implFile, "        return -1;\n");
            StreamPrintf(implFile, "    }\n");
            StreamPrintf(implFile, "}\n\n");

            StreamPrintf(headerFile, "extern char * %sToStr(%s);\n", type, type);
            StreamPrintf(implFile, "char *\n%sToStr(%s val)\n{\n", type, type);
            StreamPrintf(implFile, "    switch (val)\n    {\n");
            for (i = 0; i < ArraySize(keys); i++)
            {
                char *key = ArrayGet(keys, i);
                char *name = JsonValueAsString(JsonGet(fields, 2, key, "name"));

                StreamPrintf(implFile, "        case %s:\n", name);
                StreamPrintf(implFile, "            return \"%s\";\n", Trim('_', key));
            }
            StreamPrintf(implFile, "        default:\n");
            StreamPrintf(implFile, "            return NULL;\n");
            StreamPrintf(implFile, "    }\n");
            StreamPrintf(implFile, "}\n\n");
            StreamPutc(headerFile, '\n');

        }

        ArrayFree(keys);
    }

    StreamPrintf(headerFile, "#endif /* %s */\n", guard);

    ret = EXIT_SUCCESS;

finish:

    StreamClose(schemaFile);
    StreamClose(headerFile);
    StreamClose(implFile);

    JsonFree(schemaJson);

    return ret;
}
