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
#include <mach/at91_pmc.h>
#include <mach/at91_pio.h>
#include <mach/at91_rstc.h>
#include <mach/at91_wdt.h>
#include <mach/at91sam9_matrix.h>
#include <mach/at91sam9_sdramc.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91_lowlevel_init.h>
#include <mach/io.h>
#include <init.h>
#include <sizes.h>

void __naked __bare_init reset(void)
{
	common_reset();

	arm_setup_stack(AT91SAM9263_SRAM0_BASE + AT91SAM9263_SRAM0_SIZE - 16);

	at91sam926x_lowlevel_init(IOMEM(AT91SAM9263_BASE_PIOD), true,
				  AT91_MATRIX_EBI0CSA);

	barebox_arm_entry(AT91_CHIPSELECT_1, at91_get_sdram_size(), 0);
}
