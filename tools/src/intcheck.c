#include <Int.h>

#include <stdio.h>

#define ASSERT_SIZE(type, size) \
    if ((sizeof(type) * 8) != size) \
    { \
        fputs(#type " is not " #size " bits.\n", stderr); \
        return 1; \
    }

int
main(void)
{
    ASSERT_SIZE(Int8, 8);
    ASSERT_SIZE(UInt8, 8);

    ASSERT_SIZE(Int16, 16);
    ASSERT_SIZE(UInt16, 16);

    ASSERT_SIZE(Int32, 32);
    ASSERT_SIZE(UInt32, 32);

    return 0;
}
