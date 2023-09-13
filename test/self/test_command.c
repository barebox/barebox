// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <bselftest.h>
#include <command.h>

BSELFTEST_GLOBALS();

#if __LONG_MAX__ == 0x7ffffffff
#define LONG_MIN_STR		"-2147483648"
#define LONG_MIN_PLUS_1_STR	"-2147483647"
#define LONG_MAX_STR		 "2147483647"
#define LONG_MAX_PLUS_1_STR	 "2147483648"
#else
#define LONG_MIN_STR		"-9223372036854775808"
#define LONG_MIN_PLUS_1_STR	"-9223372036854775807"
#define LONG_MAX_STR		 "9223372036854775807"
#define LONG_MAX_PLUS_1_STR	 "9223372036854775808"
#endif

static void __assert_eq(const char *expr, bool result, const char *func, int line)
{
	int ret;

	total_tests++;

	ret = run_command(expr);
	if ((result && ret != 0) || (!result && ret != 1)) {
		failed_tests++;
		printf("%s:%d: %s: assertion failure, ret=%d\n", func, line, expr, ret);
	}
}

#define assert_eq(expr, result) __assert_eq(expr, result, __func__, __LINE__)

static void test_test_command(void)
{
	assert_eq("[  0 -eq  0 ]", true);
	assert_eq("[  0 -eq  1 ]", false);
	assert_eq("[ -1 -le  1 ]", true);
	assert_eq("[ -1 -ge -5 ]", true);

	assert_eq("[ " LONG_MIN_STR " -lt " LONG_MAX_STR " ]", true);
	assert_eq("[ " LONG_MIN_STR " -gt " LONG_MAX_STR " ]", false);

	assert_eq("[ " LONG_MIN_STR " -eq " LONG_MIN_STR " ]", true);
	assert_eq("[ " LONG_MAX_STR " -eq " LONG_MAX_STR " ]", true);
	assert_eq("[ " LONG_MIN_STR " -ne " LONG_MIN_STR " ]", false);
	assert_eq("[ " LONG_MAX_STR " -ne " LONG_MAX_STR " ]", false);

	assert_eq("[ " LONG_MIN_PLUS_1_STR " -eq " LONG_MIN_PLUS_1_STR " ]", true);
	assert_eq("[ " LONG_MAX_PLUS_1_STR " -eq " LONG_MAX_PLUS_1_STR " ]", true);
	assert_eq("[ " LONG_MIN_PLUS_1_STR " -ne " LONG_MIN_PLUS_1_STR " ]", false);
	assert_eq("[ " LONG_MAX_PLUS_1_STR " -ne " LONG_MAX_PLUS_1_STR " ]", false);

	assert_eq("[ " LONG_MIN_PLUS_1_STR " -gt " LONG_MIN_STR " ]", true);
	assert_eq("[ " LONG_MAX_PLUS_1_STR " -gt " LONG_MAX_STR " ]", true);
	assert_eq("[ " LONG_MIN_PLUS_1_STR " -lt " LONG_MIN_STR " ]", false);
	assert_eq("[ " LONG_MAX_PLUS_1_STR " -lt " LONG_MAX_STR " ]", false);
	assert_eq("[ " LONG_MIN_STR " -le " LONG_MIN_PLUS_1_STR " ]", true);
	assert_eq("[ " LONG_MAX_STR " -le " LONG_MAX_PLUS_1_STR " ]", true);
}
bselftest(core, test_test_command);
