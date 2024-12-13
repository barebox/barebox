// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>

#include <asm/armlinux.h>
#include <bbu.h>
#include <common.h>
#include <environment.h>
#include <asm/mach-types.h>
#include <init.h>
#include <mach/zynq/zynq7000-regs.h>
#include <linux/sizes.h>


static int zedboard_console_init(void)
{
	if (!of_machine_is_compatible("avnet,zynq-zed"))
		return 0;

	barebox_set_hostname("zedboard");

	bbu_register_std_file_update("SD", BBU_HANDLER_FLAG_DEFAULT,
				     "/boot/BOOT.bin", filetype_zynq_image);

	return 0;
}
console_initcall(zedboard_console_init);
