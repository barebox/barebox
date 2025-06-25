/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <mach/at91/barebox-arm.h>
#include <mach/at91/ddramc.h>

SAMA5D4_ENTRY_FUNCTION(start_sama5d4_xplained, r4)
{
	arm_cpu_lowlevel_init();

	sama5d4_barebox_entry(r4, NULL);
}
