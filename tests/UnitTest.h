#ifndef TELODENDRIA_UNITTEST_H
#define TELODENDRIA_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>

#define TEST(body) int main(void) { body; return 0; }

#define ASSERT(condition) if (!(condition)) { \
	printf("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
	exit(1); \
}

#endif
