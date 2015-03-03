/*
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2015 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
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

#include <mach/linux.h>
#include <linux/err.h>

static const void *dtb;

int barebox_register_dtb(const void *new_dtb)
{
	if (dtb)
		return -EBUSY;

	dtb = new_dtb;

	return 0;
}

static int of_sandbox_init(void)
{
	struct device_node *root;
	int ret;

	if (dtb) {
		root = of_unflatten_dtb(dtb);
	} else {
		root = of_new_node(NULL, NULL);

		ret = of_property_write_u32(root, "#address-cells", 2);
		if (ret)
			return ret;

		ret = of_property_write_u32(root, "#size-cells", 1);
		if (ret)
			return ret;
	}

	if (IS_ERR(root))
		return PTR_ERR(root);

	of_set_root_node(root);
	of_fix_tree(root);
	if (IS_ENABLED(CONFIG_OFDEVICE))
		of_probe();

	return 0;
}
core_initcall(of_sandbox_init);
