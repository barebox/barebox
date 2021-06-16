/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __BSELFTEST_H
#define __BSELFTEST_H

#include <linux/compiler.h>
#include <linux/list.h>
#include <init.h>

enum bselftest_group {
	BSELFTEST_core
};

struct selftest {
	enum bselftest_group group;
	const char *name;
	int (*func)(void);
	struct list_head list;
};

static inline int selftest_report(unsigned int total_tests, unsigned int failed_tests,
				  unsigned int skipped_tests)
{
	if (failed_tests == 0) {
		if (skipped_tests) {
			pr_info("skipped %u tests\n", skipped_tests);
			pr_info("remaining %u tests passed\n", total_tests);
		} else
			pr_info("all %u tests passed\n", total_tests);
	} else
		pr_err("failed %u out of %u tests\n", failed_tests, total_tests);

	return failed_tests ? -EINVAL : 0;
}

extern struct list_head selftests;

#define BSELFTEST_GLOBALS()			\
static unsigned int total_tests __initdata;	\
static unsigned int failed_tests __initdata;	\
static unsigned int skipped_tests __initdata

#ifdef CONFIG_SELFTEST
#define __bselftest_initcall(func) late_initcall(func)
void selftests_run(void);
#else
#define __bselftest_initcall(func)
static inline void selftests_run(void)
{
}
#endif

#define bselftest(_group, _func)				\
	static int __bselftest_##_func(void)			\
	{							\
		total_tests = failed_tests = skipped_tests = 0;	\
		_func();					\
		return selftest_report(total_tests,		\
				failed_tests,			\
				skipped_tests);			\
	}							\
	static __maybe_unused					\
	int __init _func##_bselftest_register(void)		\
	{							\
		static struct selftest this = {			\
			.group = BSELFTEST_##_group,		\
			.name = KBUILD_MODNAME,			\
			.func = __bselftest_##_func,		\
		};						\
		list_add_tail(&this.list, &selftests);		\
		return 0;					\
	}							\
	__bselftest_initcall(_func##_bselftest_register);

#endif
