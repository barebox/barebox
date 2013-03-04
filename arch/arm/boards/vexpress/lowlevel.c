/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/system_info.h>
#include <linux/amba/sp804.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	if (amba_is_arm_sp804(IOMEM(0x10011000)))
		barebox_arm_entry(0x60000000, SZ_512M, 0);
	else
		barebox_arm_entry(0x80000000, SZ_512M, 0);
}
