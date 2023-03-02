/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <mach/barebox-arm.h>

#include <mach/at91_ddrsdrc.h>
#include <mach/hardware.h>

AT91_ENTRY_FUNCTION(start_at91sam9n12ek, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(AT91SAM9N12_SRAM_BASE + AT91SAM9N12_SRAM_SIZE);

	barebox_arm_entry(AT91_CHIPSELECT_1, at91sam9n12_get_ddram_size(),
	                  NULL);
}
