#include <Int64.h>

#include <Log.h>

/* AssertEquals(actual, expected) */
int AssertEquals(char *msg, Int64 x, Int64 y)
{
    if (!Int64Eq(x, y))
    {
        Log(LOG_ERR, "%s: Expected 0x%X 0x%X, got 0x%X 0x%X", msg,
            Int64High(y), Int64Low(y),
            Int64High(x), Int64Low(x));

        return 0;
    }

    return 1;
}

int Main(void)
{
	Int64 x, y;

    Log(LOG_INFO, "sizeof(Int64) = %lu", sizeof(Int64));

#ifdef INT64_NATIVE
    Log(LOG_INFO, "Using native 64-bit integers.");
#else
    Log(LOG_INFO, "Using emulated 64-bit integers.");
#endif

    /* BSR Tests */

	x = Int64Create(0x000000FF, 0x00000000);

	y = Int64Sra(x, 4);
    AssertEquals("x >> 4", y, Int64Create(0x0000000F, 0xF0000000));

    y = Int64Sra(x, 8);
    AssertEquals("x >> 8", y, Int64Create(0x00000000, 0xFF000000));

    y = Int64Sra(x, 36);
    AssertEquals("x >> 36", y, Int64Create(0x00000000, 0x0000000F));

	x = Int64Create(0xFF000000, 0x00000000);

	y = Int64Sra(x, 4);
    AssertEquals("x >> 4", y, Int64Create(0xFFF00000, 0x00000000));

	y = Int64Sra(x, 8);
    AssertEquals("x >> 8", y, Int64Create(0xFFFF0000, 0x00000000));

	y = Int64Sra(x, 63);
    AssertEquals("x >> 63", y, Int64Create(0xFFFFFFFF, 0xFFFFFFFF));

    /* BSL Tests */

    x = Int64Create(0x00000000, 0xFF000000);

    y = Int64Sll(x, 4);
    AssertEquals("x << 4", y, Int64Create(0x0000000F, 0xF0000000));

    y = Int64Sll(x, 8);
    AssertEquals("x << 8", y, Int64Create(0x000000FF, 0x00000000));

    y = Int64Sll(x, 36);
    AssertEquals("x << 36", y, Int64Create(0xF0000000, 0x00000000));

    /* ADD Tests */

    x = Int64Create(0x00000000, 0xF0000001);
    y = Int64Create(0x00000000, 0x00000002);
    AssertEquals("0xF0000001 + 0x00000002", Int64Add(x, y), Int64Create(0x00000000, 0xF0000003));

    x = Int64Create(0x00000000, 0xF0000000);
    y = Int64Create(0x00000000, 0x10000000);
    AssertEquals("0xF0000000 + 0x10000000", Int64Add(x, y), Int64Create(0x00000001, 0x00000000));

	x = Int64Create(0, 5);
	y = Int64Neg(Int64Create(0, 10));
	AssertEquals("5 + (-10)", Int64Add(x, y), Int64Neg(Int64Create(0, 5)));

    /* SUB Tests */
    x = Int64Create(0x00000000, 0x00000005);
    y = Int64Create(0x00000000, 0x00000002);
    AssertEquals("0x00000005 - 0x00000002", Int64Sub(x, y), Int64Create(0x00000000, 0x00000003));

    x = Int64Create(0x00000001, 0x00000000);
    y = Int64Create(0x00000000, 0x00000001);
    AssertEquals("0x00000001 0x00000000 - 0x00000001", Int64Sub(x, y), Int64Create(0x00000000, 0xFFFFFFFF));

	x = Int64Create(0, 5);
	y = Int64Create(0, 10);
	AssertEquals("5 - 10", Int64Sub(x, y), Int64Neg(Int64Create(0, 5)));

	x = Int64Create(0, 5);
	y = Int64Neg(Int64Create(0, 10));
	AssertEquals("5 - (-10)", Int64Sub(x, y), Int64Create(0, 15));

    /* MUL Tests */
    x = Int64Create(0, 18);
    y = Int64Create(0, 1);
    AssertEquals("18 * 1", Int64Mul(x, y), Int64Create(0, 18));

    x = Int64Create(0, 20);
    y = Int64Create(0, 12);
    AssertEquals("20 * 12", Int64Mul(x, y), Int64Create(0, 240));

    x = Int64Create(0x00000000, 0x00000005);
    y = Int64Create(0x00000000, 0x00000005);
    AssertEquals("0x00000005 * 0x00000005", Int64Mul(x, y), Int64Create(0x00000000, 0x00000019));

    x = Int64Create(0x00000001, 0x00000000);
    y = Int64Create(0x00000000, 0x00000005);
    AssertEquals("0x00000001 0x00000000 * 0x00000005", Int64Mul(x, y), Int64Create(0x00000005, 0x00000000));

    /* DIV Tests */
    x = Int64Create(0, 12);
    y = Int64Create(0, 4);
    AssertEquals("12 / 4", Int64Div(x, y), Int64Create(0, 3));

    /* MOD Tests */
    x = Int64Create(0x000000FF, 0x00000000);
    y = Int64Create(0x00000000, 0x00000010);
    AssertEquals("0x000000FF 0x00000000 mod 0x00000010", Int64Rem(x, y), Int64Create(0, 0));

    x = Int64Create(0x00000000, 0xFF000000);
    y = Int64Create(0x00000000, 0x00000010);
    AssertEquals("0x00000000 0xFF000000 mod 0x00000010", Int64Rem(x, y), Int64Create(0, 0));

    x = Int64Create(0xFF000000, 0x00000000);
    y = Int64Create(0x00000000, 0x00000010);
    AssertEquals("0xFF000000 0x00000000 mod 0x00000010", Int64Rem(x, y), Int64Create(0, 0));

    x = Int64Create(0x00000000, 0x000000F0);
    y = Int64Create(0x00000000, 0x00000010);
    AssertEquals("0x00000000 0x000000F0 mod 0x00000010", Int64Rem(x, y), Int64Create(0, 0));

	/* TODO: Add more tests for negative multiplication, division, and mod */

	return 0;
}
