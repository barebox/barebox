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

	root = of_unflatten_dtb(boot_dtb);
	if (IS_ERR(root)) {
		ret = PTR_ERR(root);
		panic("Failed to parse DTB: %d\n", ret);
	}

	ret = of_set_root_node(root);
	if (ret)
		panic("Failed to set of root node\n");

	of_probe();

	return 0;
}
core_initcall(of_kvx_init);
