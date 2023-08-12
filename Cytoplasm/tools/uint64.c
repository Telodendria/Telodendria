#include <UInt64.h>

#include <Log.h>

/* AssertEquals(actual, expected) */
int AssertEquals(char *msg, UInt64 x, UInt64 y)
{
    if (!UInt64Eq(x, y))
    {
        Log(LOG_ERR, "%s: Expected 0x%X 0x%X, got 0x%X 0x%X", msg,
            UInt64High(y), UInt64Low(y),
            UInt64High(x), UInt64Low(x));

        return 0;
    }

    return 1;
}

int Main(void)
{
	UInt64 x, y;

    Log(LOG_INFO, "sizeof(UInt64) = %lu", sizeof(UInt64));

#ifdef UINT64_NATIVE
    Log(LOG_INFO, "Using native 64-bit integers.");
#else
    Log(LOG_INFO, "Using emulated 64-bit integers.");
#endif

    /* BSR Tests */

	x = UInt64Create(0x000000FF, 0x00000000);

	y = UInt64Srl(x, 4);
    AssertEquals("x >> 4", y, UInt64Create(0x0000000F, 0xF0000000));

    y = UInt64Srl(x, 8);
    AssertEquals("x >> 8", y, UInt64Create(0x00000000, 0xFF000000));

    y = UInt64Srl(x, 36);
    AssertEquals("x >> 36", y, UInt64Create(0x00000000, 0x0000000F));

    /* BSL Tests */

    x = UInt64Create(0x00000000, 0xFF000000);

    y = UInt64Sll(x, 4);
    AssertEquals("x << 4", y, UInt64Create(0x0000000F, 0xF0000000));

    y = UInt64Sll(x, 8);
    AssertEquals("x << 8", y, UInt64Create(0x000000FF, 0x00000000));

    y = UInt64Sll(x, 36);
    AssertEquals("x << 36", y, UInt64Create(0xF0000000, 0x00000000));

    /* ADD Tests */

    x = UInt64Create(0x00000000, 0xF0000001);
    y = UInt64Create(0x00000000, 0x00000002);
    AssertEquals("0xF0000001 + 0x00000002", UInt64Add(x, y), UInt64Create(0x00000000, 0xF0000003));

    x = UInt64Create(0x00000000, 0xF0000000);
    y = UInt64Create(0x00000000, 0x10000000);
    AssertEquals("0xF0000000 + 0x10000000", UInt64Add(x, y), UInt64Create(0x00000001, 0x00000000));

    /* SUB Tests */
    x = UInt64Create(0x00000000, 0x00000005);
    y = UInt64Create(0x00000000, 0x00000002);
    AssertEquals("0x00000005 - 0x00000002", UInt64Sub(x, y), UInt64Create(0x00000000, 0x00000003));

    x = UInt64Create(0x00000001, 0x00000000);
    y = UInt64Create(0x00000000, 0x00000001);
    AssertEquals("0x00000001 0x00000000 - 0x00000001", UInt64Sub(x, y), UInt64Create(0x00000000, 0xFFFFFFFF));

    /* MUL Tests */
    x = UInt64Create(0, 18);
    y = UInt64Create(0, 1);
    AssertEquals("18 * 1", UInt64Mul(x, y), UInt64Create(0, 18));

    x = UInt64Create(0, 20);
    y = UInt64Create(0, 12);
    AssertEquals("20 * 12", UInt64Mul(x, y), UInt64Create(0, 240));

    x = UInt64Create(0x00000000, 0x00000005);
    y = UInt64Create(0x00000000, 0x00000005);
    AssertEquals("0x00000005 * 0x00000005", UInt64Mul(x, y), UInt64Create(0x00000000, 0x00000019));

    x = UInt64Create(0x00000001, 0x00000000);
    y = UInt64Create(0x00000000, 0x00000005);
    AssertEquals("0x00000001 0x00000000 * 0x00000005", UInt64Mul(x, y), UInt64Create(0x00000005, 0x00000000));

    /* DIV Tests */
    x = UInt64Create(0, 12);
    y = UInt64Create(0, 4);
    AssertEquals("12 / 4", UInt64Div(x, y), UInt64Create(0, 3));

    /* MOD Tests */
    x = UInt64Create(0x000000FF, 0x00000000);
    y = UInt64Create(0x00000000, 0x00000010);
    AssertEquals("0x000000FF 0x00000000 mod 0x00000010", UInt64Rem(x, y), UInt64Create(0, 0));

    x = UInt64Create(0x00000000, 0xFF000000);
    y = UInt64Create(0x00000000, 0x00000010);
    AssertEquals("0x00000000 0xFF000000 mod 0x00000010", UInt64Rem(x, y), UInt64Create(0, 0));

    x = UInt64Create(0xFF000000, 0x00000000);
    y = UInt64Create(0x00000000, 0x00000010);
    AssertEquals("0xFF000000 0x00000000 mod 0x00000010", UInt64Rem(x, y), UInt64Create(0, 0));

    x = UInt64Create(0x00000000, 0x000000F0);
    y = UInt64Create(0x00000000, 0x00000010);
    AssertEquals("0x00000000 0x000000F0 mod 0x00000010", UInt64Rem(x, y), UInt64Create(0, 0));

	return 0;
}
