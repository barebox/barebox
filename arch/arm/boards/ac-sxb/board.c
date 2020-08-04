// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2017 Atlas Copco Industrial Technique
 */

#include <common.h>
#include <init.h>
#include <mach/bbu.h>

static int sxb_coredevices_init(void)
{
	if (!of_machine_is_compatible("ac,imx7d-sxb"))
		return 0;

	imx7_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", 0);

	return 0;
}
coredevice_initcall(sxb_coredevices_init);
