// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <common.h>
#include <init.h>
#include <of.h>

static int of_kvx_init(void)
{
	return barebox_register_fdt(boot_dtb);
}
core_initcall(of_kvx_init);
