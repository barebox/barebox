// SPDX-License-Identifier: GPL-2.0-only

#include "linux/kconfig.h"
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <abort.h>
#include <asm/system.h>
#include <common.h>
#include <bselftest.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <of.h>

BSELFTEST_GLOBALS();

#define CANARY (0xDEADC0DE)

static void write_abort_mask(void)
{
	if (IS_ENABLED(CONFIG_ARCH_HAS_ASYNC_ABORT_MASK))
		async_abort_mask();
	else
		data_abort_mask();
}

static int write_abort_unmask(void)
{
	if (IS_ENABLED(CONFIG_ARCH_HAS_ASYNC_ABORT_MASK))
		return async_abort_unmask();
	else
		return data_abort_unmask();
}

static void test_optee_core_memory_isolation(void)
{
	struct resource *optee_core;
	void __iomem *start, *end;
	bool failed = false;

	optee_core =
		resource_find_by_name_prefix(&iomem_resource, "optee_core");
	if (!assert_cond(optee_core != NULL))
		return;

	assert_cond(optee_core->attrs & MEMATTR_UC);
	assert_cond(optee_core->attrs & MEMATTR_XP);
	assert_cond(optee_core->type == MEMTYPE_RESERVED);

	start = IOMEM(optee_core->start);
	end = IOMEM(optee_core->end);

	while (start <= (end - sizeof(u32))) {
		write_abort_mask();
		writel(CANARY, start);
		if (!write_abort_unmask()) {
			pr_err("Secure world memory writable at: %p\n",
			       (void *)start);
			failed = true;
		}

		data_abort_mask();
		if (readl(start) == CANARY || !data_abort_unmask()) {
			pr_err("Secure world memory readable at: %p\n",
			       (void *)start);
			failed = true;
		}

		start += PAGE_SIZE;
	}

	assert_cond(!failed);
}

static void test_optee(void)
{
	test_optee_core_memory_isolation();
}
bselftest_destructive(core, test_optee);
