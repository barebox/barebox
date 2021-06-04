/* SPDX-License-Identifier: GPL-2.0 */

#define pr_fmt(fmt) "bselftest: " fmt

#include <common.h>
#include <bselftest.h>

LIST_HEAD(selftests);

void selftests_run(void)
{
	struct selftest *test;
	int err = 0;

	pr_notice("Configured tests will run now\n");

	list_for_each_entry(test, &selftests, list)
		err |= test->func();

	if (err)
		pr_err("Some selftests failed\n");
}
