/*
 * Copyright (C) 2013 Rostislav Lisovy <lisovy@gmail.com>, PiKRON s.r.o.
 *
 * Board specific file for Voipac X53-DMM-668 module equipped
 * with i.MX53 CPU
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <sizes.h>

#include <generated/mach-types.h>
#include <mach/imx5.h>
#include <asm/armlinux.h>
#include <mach/bbu.h>

static int vmx53_late_init(void)
{
	if (!of_machine_is_compatible("voipac,imx53-dmm-668"))
			return 0;

	armlinux_set_architecture(MACH_TYPE_VMX53);

	barebox_set_model("Voipac VMX53");
	barebox_set_hostname("vmx53");

	imx53_bbu_internal_nand_register_handler("nand",
		BBU_HANDLER_FLAG_DEFAULT, SZ_512K);

	return 0;
}
late_initcall(vmx53_late_init);

static int vmx53_postcore_init(void)
{
	if (!of_machine_is_compatible("voipac,imx53-dmm-668"))
		return 0;

	imx53_init_lowlevel(800);

	return 0;
}
postcore_initcall(vmx53_postcore_init);
