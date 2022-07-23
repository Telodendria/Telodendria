#ifndef TELODENDRIA_CONFIG_H
#define TELODENDRIA_CONFIG_H

#include <stdio.h>

#include <HashMap.h>
#include <Array.h>

typedef struct ConfigDirective ConfigDirective;

typedef struct ConfigParseResult ConfigParseResult;

extern ConfigParseResult *
ConfigParse(FILE *stream);

extern unsigned int
ConfigParseResultOk(ConfigParseResult *result);

extern size_t
ConfigParseResultLineNumber(ConfigParseResult *result);

extern HashMap *
ConfigParseResultGet(ConfigParseResult *result);

extern void
ConfigParseResultFree(ConfigParseResult *result);

extern Array *
ConfigValuesGet(ConfigDirective *directive);

extern HashMap *
ConfigChildrenGet(ConfigDirective *directive);

extern void
ConfigFree(HashMap *conf);

#endif /* TELODENDRIA_CONFIG_H */
