/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <mach/barebox-arm.h>

#include <mach/hardware.h>
#include <mach/at91_ddrsdrc.h>

AT91_ENTRY_FUNCTION(start_at91sam9m10g45ek, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(AT91SAM9G45_SRAM_BASE + AT91SAM9G45_SRAM_SIZE);

	barebox_arm_entry(AT91_CHIPSELECT_6, at91sam9g45_get_ddram_size(1),
	                  NULL);
}
