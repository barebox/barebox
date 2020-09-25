// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on arch/arm/cpu/dtb.c:
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <init.h>
#include <of.h>
#include <memory.h>
#include <asm/addrspace.h>

void *glob_fdt;
u32 glob_fdt_size;

void of_add_memory_bank(struct device_node *node, bool dump, int r,
		u64 base, u64 size)
{
	static char str[12];

	if (IS_ENABLED(CONFIG_MMU)) {
		sprintf(str, "kseg0_ram%d", r);
		barebox_add_memory_bank(str, CKSEG0 | base, size);
	} else {
		sprintf(str, "kseg1_ram%d", r);
		barebox_add_memory_bank(str, CKSEG1 | base, size);
	}

	if (dump)
		pr_info("%s: %s: 0x%llx@0x%llx\n", node->name, str, size, base);
}

extern char __dtb_start[];

static int of_mips_init(void)
{
	struct device_node *root;
	void *fdt;

	fdt = glob_fdt;
	if (!fdt)
		fdt = __dtb_start;

	barebox_register_fdt(fdt);

	return 0;
}
core_initcall(of_mips_init);
