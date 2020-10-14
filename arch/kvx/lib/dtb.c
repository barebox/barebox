// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <common.h>
#include <init.h>
#include <of.h>

static int of_kvx_init(void)
{
	int ret;
	struct device_node *root;

	barebox_register_fdt(boot_dtb);

	return 0;
}
core_initcall(of_kvx_init);
