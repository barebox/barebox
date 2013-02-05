/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
  */

#define __LOWLEVEL_INIT__

#include <common.h>
#include <asm/system.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/at91sam9_ddrsdr.h>
#include <init.h>
#include <sizes.h>

void __naked __bare_init barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(AT91SAM9X5_SRAM_BASE + AT91SAM9X5_SRAM_SIZE - 16);

	barebox_arm_entry(AT91_CHIPSELECT_1, at91sam9x5_get_ddram_size(), 0);
}
