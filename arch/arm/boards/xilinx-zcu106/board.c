// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021, WolfVision GmbH
 * Author: Michael Riesch <michael.riesch@wolfvision.net>
 *
 * Based on the barebox ZCU104 board support code.
 */

#include <common.h>
#include <init.h>
#include <mach/zynqmp-bbu.h>

static int zcu106_register_update_handler(void)
{
	if (!of_machine_is_compatible("xlnx,zynqmp-zcu106"))
		return 0;

	return zynqmp_bbu_register_handler("SD", "/boot/BOOT.BIN",
					   BBU_HANDLER_FLAG_DEFAULT);
}
device_initcall(zcu106_register_update_handler);
