#include <Memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct MemoryInfo
{
	size_t size;
	const char *file;
	const char *func;
	int line;
	void *pointer;

	MemoryInfo *next;
	MemoryInfo *prev;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static MemoryInfo *lastAllocation = NULL;

void *
MemoryAllocate(size_t size, const char *file, int line, const char *func)
{
	void *p;
	MemoryInfo *a;

	pthread_mutex_lock(&lock);

	p = malloc(size);
	if (!p)
	{
		pthread_mutex_unlock(&lock);
		return NULL;
	}

	a = malloc(sizeof(MemoryInfo));
	if (!a)
	{
		free(p);
		pthread_mutex_unlock(&lock);
		return NULL;
	}

	a->size = size;
	a->file = file;
	a->line = line;
	a->func = func;
	a->pointer = p;
	a->next = NULL;
	a->prev = lastAllocation;

	if (lastAllocation)
	{
		lastAllocation->next = a;
	}

	lastAllocation = a;

	pthread_mutex_unlock(&lock);
	return p;
}

void *
MemoryReallocate(void *p, size_t size)
{
	MemoryInfo *a;
	void *new = NULL;

	pthread_mutex_lock(&lock);

	a = lastAllocation;
	while (a)
	{
		if (a->pointer == p)
		{
			new = realloc(p, size);
			if (new)
			{
				a->pointer = new;
				a->size = size;
			}

			break;
		}

		a = a->prev;
	}

	pthread_mutex_unlock(&lock);

	return new;
}

void
MemoryFree(void *p)
{
	MemoryInfo *a;

	pthread_mutex_lock(&lock);

	a = lastAllocation;

	while (a)
	{
		if (a->pointer == p)
		{
			if (a->prev)
			{
				a->prev->next = a->next;
			}
			else
			{
				lastAllocation = a->next;
			}

			if (a->next)
			{
				a->next->prev = a->prev;
			}
			else
			{
				lastAllocation = a->prev;
			}

			free(a);
			free(p);

			break;
		}
			
		a = a->prev;
	}

	pthread_mutex_unlock(&lock);
}

size_t
MemoryAllocated(void)
{
	MemoryInfo *a;
	size_t total = 0;

	pthread_mutex_lock(&lock);

	a = lastAllocation;
	while (a)
	{
		total += a->size;
		a = a->prev;
	}

	pthread_mutex_unlock(&lock);

	return total;
}

void
MemoryFreeAll(void)
{
	MemoryInfo *a;

	pthread_mutex_lock(&lock);

	a = lastAllocation;
	while (a)
	{
		MemoryInfo *prev = a->prev;

		free(a->pointer);
		free(a);

		a = prev;
	}

	lastAllocation = NULL;

	pthread_mutex_unlock(&lock);
}

MemoryInfo *
MemoryInfoGet(void *p)
{
	MemoryInfo *a;

	pthread_mutex_lock(&lock);

	a = lastAllocation;
	while (a)
	{
		if (a->pointer == p)
		{
			break;
		}

		a = a->prev;
	}

	return a;
}

size_t
MemoryInfoGetSize(MemoryInfo *a)
{
	if (!a)
	{
		return 0;
	}

	return a->size;
}

const char *
MemoryInfoGetFile(MemoryInfo *a)
{
	if (!a)
	{
		return NULL;
	}

	return a->file;
}

const char *
MemoryInfoGetFunc(MemoryInfo *a)
{
	if (!a)
	{
		return NULL;
	}

	return a->func;
}

int
MemoryInfoGetLine(MemoryInfo *a)
{
	if (!a)
	{
		return -1;
	}

	return a->line;
}

void *
MemoryInfoGetPointer(MemoryInfo *a)
{
	if (!a)
	{
		return NULL;
	}

	return a->pointer;
}

void
MemoryIterate(void (*iterFunc)(MemoryInfo *, void *), void *args)
{
	MemoryInfo *a;

	pthread_mutex_lock(&lock);

	a = lastAllocation;
	while (a)
	{
		iterFunc(a, args);
		a = a->prev;
	}

	pthread_mutex_unlock(&lock);
}
