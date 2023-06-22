// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright (C) 2003 Hewlett-Packard
/*
 * Code contributed by David Mosberger-Tang <davidm@hpl.hp.com>
 * to libunwind - a platform-independent unwind library
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <string.h>
#include <asm/setjmp.h>

BSELFTEST_GLOBALS();

static __noreturn void raise_longjmp(jmp_buf jbuf, int i, int n)
{
	while (i < n)
		raise_longjmp(jbuf, i + 1, n);

	longjmp(jbuf, n);
}

static jmp_buf jbuf;

static void __noreturn initjmp_entry(void)
{
	volatile u32 arr[256];
	int i;

	for (i = 0; i < ARRAY_SIZE(arr); i++)
		writel(i, &arr[i]);

	/* ensure arr[] is allocated on stack */
	OPTIMIZER_HIDE_VAR(i);
	if (i == 0)
		initjmp_entry();

	longjmp(jbuf, 0x1337);
}

static void test_setjmp(void)
{
	void *stack;
	jmp_buf ijbuf;
	volatile int i;
	int ret;

	total_tests += 20;

	for (i = 0; i < 10; ++i) {
		if ((ret = setjmp(jbuf))) {
			pr_debug("%s: secondary setjmp() return, ret=%d\n",
				 __func__, ret);

			if (ret != i + 1) {
				printf("%s: setjmp() returned %d, expected %d\n",
					 __func__, ret, i + 1);
				failed_tests += 2;
			}
			continue;
		}

		pr_debug("%s.%d: done with setjmp(); calling children\n",
			 __func__, i + 1);

		raise_longjmp(jbuf, 0, i + 1);

		printf("%s: raise_longjmp() returned unexpectedly\n", __func__);
		failed_tests++;
	}

	stack = memalign(16, CONFIG_STACK_SIZE);
	if (WARN_ON(!stack)) {
		skipped_tests++;
		return;
	}

	pr_debug("%s: testing initjmp\n", __func__);

	total_tests += 1;

	/* initialize new context with fresh stack */
	initjmp(ijbuf, initjmp_entry, stack + CONFIG_STACK_SIZE);

	switch (setjmp(jbuf)) {
	case 0x1337:
		break;
	case 0:
		longjmp(ijbuf, 0x42);
	default:
		failed_tests++;
	}

	free(stack);
}
bselftest(core, test_setjmp);
