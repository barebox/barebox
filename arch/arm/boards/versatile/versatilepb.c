// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2010 B Labs Ltd,
 * http://l4dev.org
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 */

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/system_info.h>
#include <asm/mach-types.h>
#include <mach/versatile/platform.h>
#include <environment.h>
#include <linux/sizes.h>
#include <platform_data/eth-smc91111.h>

static int vpb_console_init(void)
{
	char *hostname = "versatilepb-unknown";
	char *model = "ARM Versatile PB";

	if (!of_machine_is_compatible("arm,versatile-pb") &&
	    !of_machine_is_compatible("arm,versatile-ab"))
		return 0;

	if (cpu_is_arm926()) {
		hostname = "versatilepb-arm926";
		model = "ARM Versatile PB (arm926)";
	} else if (cpu_is_arm1176()) {
		hostname = "versatilepb-arm1176";
		model = "ARM Versatile PB (arm1176)";
	}

	armlinux_set_architecture(MACH_TYPE_VERSATILE_PB);
	barebox_set_hostname(hostname);
	barebox_set_model(model);

	return 0;
}
console_initcall(vpb_console_init);
