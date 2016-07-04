/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/system_info.h>

void barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	arm_setup_stack(0x40000000 + SZ_2G - SZ_16K);

	barebox_arm_entry(0x40000000, SZ_2G, NULL);
}
