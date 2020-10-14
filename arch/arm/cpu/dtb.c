// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <common.h>
#include <init.h>
#include <of.h>
#include <asm/barebox-arm.h>

static int of_arm_init(void)
{
	struct device_node *root;
	void *fdt;

	/* See if we already have a dtb */
	root = of_get_root_node();
	if (root)
		return 0;

	/* See if we are provided a dtb in boarddata */
	fdt = barebox_arm_boot_dtb();
	if (fdt)
		pr_debug("using boarddata provided DTB\n");

	if (!fdt) {
		pr_debug("No DTB found\n");
		return 0;
	}

	barebox_register_fdt(fdt);

	return 0;
}
core_initcall(of_arm_init);
