#ifndef TELODENDRIA_ARRAY_H
#define TELODENDRIA_ARRAY_H

#include <stddef.h>

typedef struct Array Array;

extern Array *
 ArrayCreate(void);

extern size_t
 ArraySize(Array * array);

extern void *
 ArrayGet(Array * array, size_t index);

extern int
 ArrayInsert(Array *, void *value, size_t index);

extern int
 ArrayAdd(Array * array, void *value);

extern void *
 ArrayDelete(Array * array, size_t index);

extern void
 ArraySort(Array *, int (*compare) (void *, void *));

extern void
 ArrayFree(Array * array);

extern int
 ArrayTrim(Array * array);

#endif
