// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <mach/zynqmp-bbu.h>

static int zcu104_register_update_handler(void)
{
	if (!of_machine_is_compatible("xlnx,zynqmp-zcu104"))
		return 0;

	return zynqmp_bbu_register_handler("SD", "/boot/BOOT.BIN",
					   BBU_HANDLER_FLAG_DEFAULT);
}
device_initcall(zcu104_register_update_handler);
