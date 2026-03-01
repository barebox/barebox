// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <boot.h>
#include <envfs.h>
#include <init.h>

BSELFTEST_GLOBALS();

static void test_blspec(void)
{
	struct bootentries *entries;
	struct bootentry *entry;
	int ret, i = 0;
	const char *expected[] = {
		"/env/data/test/loader/entries/boardb.conf",
		"/env/data/test/loader/entries/boardc.conf",
		"/env/data/test/loader/entries/boarda.conf",
		"/env/data/test/loader/entries/boardd.conf",
	};

	entries = bootentries_alloc();

	ret = bootentry_create_from_name(entries, "/env/data/test");
	if (!assert_inteq(ret, 4))
		return;

	if (!assert_cond(!list_empty(&entries->entries)))
		return;

	bootentries_for_each_entry(entries, entry) {
		assert_streq(expected[i], entry->path);
		i++;
	}
}
bselftest(parser, test_blspec);

static int test_blspec_env_init(void)
{
	defaultenv_append_directory(defaultenv_blspec_test);
	return 0;
}
late_initcall(test_blspec_env_init);
