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

void __bare_init at91sam9263_lowlevel_init(void)
{
	struct at91sam926x_lowlevel_cfg cfg;

	cfg.pio = IOMEM(AT91SAM9263_BASE_PIOD);
	cfg.sdramc = IOMEM(AT91SAM9263_BASE_SDRAMC0);
	cfg.ebi_pio_is_peripha = true;
	cfg.matrix_csa = AT91_MATRIX_EBI0CSA;

	at91sam926x_lowlevel_init(&cfg);

	barebox_arm_entry(AT91_CHIPSELECT_1, at91_get_sdram_size(cfg.sdramc), 0);
}

void __naked __bare_init barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(AT91SAM9263_SRAM0_BASE + AT91SAM9263_SRAM0_SIZE - 16);

	at91sam9263_lowlevel_init();
}
