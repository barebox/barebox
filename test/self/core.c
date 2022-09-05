/* SPDX-License-Identifier: GPL-2.0 */

#define pr_fmt(fmt) "bselftest: " fmt

#include <common.h>
#include <bselftest.h>

LIST_HEAD(selftests);

int selftest_run(struct selftest *test)
{
	int err;

	test->running = true;
	err = test->func();
	test->running = false;

	return err;
}

bool selftest_is_running(struct selftest *test)
{
	if (test)
		return test->running;

	list_for_each_entry(test, &selftests, list) {
		if (selftest_is_running(test))
			return true;
	}

	return false;
}

void selftests_run(void)
{
	struct selftest *test;
	int err = 0;

	pr_notice("Configured tests will run now\n");

	list_for_each_entry(test, &selftests, list)
		err |= selftest_run(test);

	if (err)
		pr_err("Some selftests failed\n");
}
