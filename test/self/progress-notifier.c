// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <progress.h>

BSELFTEST_GLOBALS();

static void __ok(bool cond, const char *func, int line)
{
	total_tests++;
	if (!cond) {
		failed_tests++;
		printf("%s:%d: assertion failure\n", func, line);
	}
}

#define ok(cond) \
	__ok(cond, __func__, __LINE__)

static unsigned long stage;
static const void *prefix;
static int counter;

static int dummy_notifier(struct notifier_block *r, unsigned long _stage, void *_prefix)
{
       prefix = _prefix;
       stage = _stage;
       counter++;
       return 0;
}

static struct notifier_block dummy_nb =  {
       .notifier_call = dummy_notifier
};

static void test_dummy_notifier(void)
{
	const char *arg = "ARGUMENT";
	int local_counter = 0;

	stage = 0;
	prefix = NULL;
	counter = 0;

	progress_register_client(&dummy_nb);
	ok(stage == 0);
	ok(prefix == NULL);
	ok(counter == local_counter);
	progress_notifier_call_chain(1, arg);

	if (IS_ENABLED(CONFIG_PROGRESS_NOTIFIER)) {
		ok(stage == 1);
		ok(prefix == arg);
		ok(counter == ++local_counter);
		progress_notifier_call_chain(0, NULL);
		local_counter++;
	} else {
		total_tests += 2;
		skipped_tests += 2;
	}

	ok(stage == 0);
	ok(prefix == NULL || *(const char *)prefix == '\0');
	ok(counter == local_counter);
	progress_unregister_client(&dummy_nb);

	ok(stage == 0);
	ok(prefix == NULL || *(const char *)prefix == '\0');
	ok(counter == local_counter);
}

static void test_notifier(void)
{
	test_dummy_notifier();
}
bselftest(core, test_notifier);
