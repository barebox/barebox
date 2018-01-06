/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

#include <mach/at91sam9_sdramc.h>
#include <mach/at91sam9260.h>
#include <mach/hardware.h>

void __naked __bare_init barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(AT91SAM9260_SRAM_BASE + AT91SAM9260_SRAM_SIZE - 16);

	barebox_arm_entry(AT91_CHIPSELECT_1,
			  at91_get_sdram_size(IOMEM(AT91SAM9260_BASE_SDRAMC)),
			  NULL);
}
