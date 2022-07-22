// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <environment.h>
#include <bselftest.h>
#include <linux/sizes.h>

BSELFTEST_GLOBALS();

static int strequal(const char *a, const char *b)
{
	if (!a || !b)
		return a == b;

	return !strcmp(a, b);
}

static void __expect_getenv(const char *var, const char *expect,
			    const char *func, int line)
{
	const char *val;

	total_tests++;

	val = getenv(var);
	if (!IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES)) {
		if (val == NULL) {
			skipped_tests++;
			return;
		}
	}

	if (!strequal(val, expect)) {
		failed_tests++;
		printf("%s:%d: failure: getenv(%s) == \"%s\", but \"%s\" expected\n",
		       func, line, var, val ?: "<NULL>", expect ?: "<NULL>");
	}
}

#define expect_getenv(v, e) __expect_getenv(v, e, __func__, __LINE__)

static void test_envvar(void)
{

	if (!IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
		pr_info("built without environment variable support: Skipping tests\n");

	expect_getenv("__TEST_VAR1", NULL);

	setenv("__TEST_VAR1", "VALUE1");

	expect_getenv("__TEST_VAR1", "VALUE1");

	unsetenv("__TEST_VAR1");

	expect_getenv("__TEST_VAR1", NULL);

	setenv("__TEST_VAR1", "VALUE1");

	expect_getenv("__TEST_VAR1", "VALUE1");

	setenv("__TEST_VAR1", "VALUE2");

	expect_getenv("__TEST_VAR1", "VALUE2");

	setenv("__TEST_VAR1", "1337");

	expect_getenv("__TEST_VAR1", "1337");

	pr_setenv("__TEST_VAR1", "0x%s", "1337");

	expect_getenv("__TEST_VAR1", "0x1337");

	pr_setenv("__TEST_VAR1", "%ux1%c%x", 0, '3', 0x37);

	expect_getenv("__TEST_VAR1", "0x1337");

	unsetenv("__TEST_VAR1");
}
bselftest(core, test_envvar);
