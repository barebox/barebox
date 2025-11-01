// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "range: " fmt

#include <common.h>
#include <bselftest.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <stdio.h>
#include <range.h>

BSELFTEST_GLOBALS();

static void __expect_bool(const char *msg, bool got, bool want)
{
	total_tests++;

	if (got != want) {
		failed_tests++;
		pr_err("FAIL: %s — got %d want %d\n", msg, got, want);
	}
}

#define expect_bool(got, want, type, a, b, c, d) do {		\
	char _buf[256];						\
	snprintf(_buf, sizeof(_buf), "%d: " type		\
		 " [%#llx, %#llx] vs [%#llx, %#llx]", __LINE__,	\
		 (u64)(a), (u64)(b), (u64)(c), (u64)(d));	\
	__expect_bool(_buf, got, want);				\
} while (0)

#define TEST_INCL(a, b, c, d, expect)				\
	expect_bool(region_overlap_end_inclusive(a, b, c, d),	\
		    expect, "incl", a, b, c, d);

#define TEST_EXCL(a, b, c, d, expect)				\
	expect_bool(region_overlap_end_exclusive(a, b, c, d),	\
		    expect, "excl", a, b, c, d);

#define TEST_SIZE(a, lena, b, lenb, expect) \
	expect_bool(region_overlap_size(a, lena, b, lenb),	\
		    expect, "size", a, lena, b, lenb)

static void test_region_overlap(void)
{
	TEST_INCL(0u, 5u, 5u, 10u, true);
	TEST_INCL(0u, 4u, 5u, 10u, false);
	TEST_INCL(10u, 20u, 12u, 15u, true);
	TEST_INCL(20u, 10u, 12u, 15u, false);

	TEST_EXCL(0ull, 5ull, 5ull, 10ull, false);
	TEST_EXCL(0u, 6u, 5u, 10u, true);
	TEST_EXCL(5u, 5u, 5u, 10u, false);
	TEST_EXCL(5u, 10u, 7u, 7u, false);
	TEST_EXCL(3u, 8u, 3u, 8u, true);
	TEST_EXCL(0u, 0u, 0u, 1u, true);
	TEST_EXCL(0xFFFFFF00, 0xFFFFFF10,
		  0xFFFFF000, 0xFFFFFFF0, true);
	TEST_EXCL(0u, 1u, 1u, 1u, false);

	TEST_SIZE(0u, 0u, 0u, 10u, false);
	TEST_SIZE(0u, 10u, 5u, 0u, false);
	TEST_SIZE(0u, 5u, 5u, 5u, false);
	TEST_SIZE(0u, 6u, 5u, 5u, true);
	TEST_SIZE(0u, U64_MAX, 123u, 1u, true);

	TEST_EXCL(0u, 0u, 0u, 0u, true);
	TEST_EXCL(0ull, 0ull, 0ull, 0ull, true);


	TEST_EXCL(0xF0000000u, 0u,
		  (u32)(SZ_4G + SZ_1G), (u32)(SZ_4G + SZ_2G), false);
	TEST_EXCL(0xF0000000ul, 0ul,
		  (ulong)(SZ_4G + SZ_1G), (ulong)(SZ_4G + SZ_2G),
		  sizeof(ulong) == sizeof(u64));
	TEST_EXCL(0xF0000000ull, 0ull,
		  SZ_4G + SZ_1G, SZ_4G + SZ_2G, true);

	TEST_EXCL(1u, 1u, 1u, 1u, false);

	TEST_EXCL(0xFFFFFFFEu, 0u, 0u, 1u, false);

	TEST_EXCL(U32_MAX / 2, U32_MAX + 1,
		  U32_MAX / 2 + 0x10, U32_MAX / 2 + 0x100, true);

	TEST_SIZE(1u, U64_MAX, 1u, 1u, true);
	TEST_SIZE(1u, U64_MAX, 10u, 10u, true);

	TEST_SIZE(0xFFFFFFFFFFFFFFE0ull, 0x20ull,
		  0xFFFFFFFFFFFFFFFFull, 0x0ull, false);

	TEST_SIZE(0xFFFFFFFFFFFFFFE0ull, 0x20ull,
		  0xFFFFFFFFFFFFFFFFull, 0x0ull, false);

	TEST_SIZE(0xFFFFFFFFFFFFFFE0ull, 0x20ull,
		  0xFFFFFFFFFFFFFFFCull, 0x1ull, true);

	TEST_SIZE(0xFFFFFFFFFFFFFFE0ull, 0x20ull,
		  0xFFFFFFFFFFFFFFFEull, 0x1ull, true);

	TEST_SIZE(0xFFFFFFFFFFFFFFE0ull, 0x20ull,
		  0ull, 10ull, false);
}

bselftest(core, test_region_overlap);
