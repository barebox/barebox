/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on arch/arm/cpu/dtb.c:
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <init.h>
#include <of.h>
#include <memory.h>
#include <asm/addrspace.h>

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

	root = of_get_root_node();
	if (root)
		return 0;

	root = of_unflatten_dtb(__dtb_start);
	if (!IS_ERR(root)) {
		pr_debug("using internal DTB\n");
		of_set_root_node(root);
		if (IS_ENABLED(CONFIG_OFDEVICE))
			of_probe();
	}

	return 0;
}
core_initcall(of_mips_init);
