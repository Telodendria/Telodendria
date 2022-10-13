#ifndef TELODENDRIA_MEMORY_H
#define TELODENDRIA_MEMORY_H

#include <stddef.h>

typedef enum MemoryAction
{
	MEMORY_ALLOCATE,
	MEMORY_REALLOCATE,
	MEMORY_FREE
} MemoryAction;

#define Malloc(x) MemoryAllocate(x, __FILE__, __LINE__, __FUNCTION__)
#define Realloc(x, s) MemoryReallocate(x, s)
#define Free(x) MemoryFree(x)

typedef struct MemoryInfo MemoryInfo;

extern void *MemoryAllocate(size_t, const char *, int, const char *);
extern void *MemoryReallocate(void *, size_t);
extern void MemoryFree(void *);

extern size_t MemoryAllocated(void);
extern void MemoryFreeAll(void);

extern MemoryInfo * MemoryInfoGet(void *);

extern size_t MemoryInfoGetSize(MemoryInfo *);
extern const char * MemoryInfoGetFile(MemoryInfo *);
extern const char * MemoryInfoGetFunc(MemoryInfo *);
extern int MemoryInfoGetLine(MemoryInfo *);
extern void * MemoryInfoGetPointer(MemoryInfo *);

extern void MemoryIterate(void (*)(MemoryInfo *, void *), void *);

extern void MemoryHook(void (*)(MemoryAction, MemoryInfo *, void *), void *);

#endif
