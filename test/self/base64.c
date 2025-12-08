// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <base64.h>
#include <string.h>

BSELFTEST_GLOBALS();

static void __expect_base64(const char *func, int line, int dst_len,
			   const char *src, int expect_len, const char *expect)
{
	int ret;
	char *buf = strdup("canary");
	bool fail = false;

	total_tests++;
	ret = decode_base64(buf, dst_len, src);
	if (!streq_ptr(buf, expect)) {
		fail = true;
		printf("%s:%d: got '%s', but '%s' expected\n", func, line, buf,
		       expect);
	}
	if (ret != expect_len) {
		fail = true;
		printf("%s:%d: got length %i, but %i expected\n", func, line,
		       ret, expect_len);
	}
	if (fail)
		failed_tests++;
	free(buf);
}

#define expect_base64(dst_len, src, expect_len, expect) \
	__expect_base64(__func__, __LINE__, dst_len, src, expect_len, expect)

static void test_base64(void)
{
	expect_base64(1, "QUJD", 1, "Aanary");
	expect_base64(5, "QUJD", 3, "ABCary");
	expect_base64(5, "$UJD", 0, "canary");
}
bselftest(parser, test_base64);
