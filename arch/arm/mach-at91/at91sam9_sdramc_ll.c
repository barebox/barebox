// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2006, Atmel Corporation
 */

#include <mach/at91/at91sam9_sdramc.h>
#include <mach/at91/early_udelay.h>

static inline void sdramc_wr(const struct at91sam9_sdramc_config *config,
			     unsigned int offset,
			     const unsigned int value)
{
	writel(value, config->sdramc + offset);
}

int at91sam9_sdramc_initialize(const struct at91sam9_sdramc_config *config,
			       unsigned int sdram_address)
{
	unsigned int i;

	/* Step#1 SDRAM feature must be in the configuration register */
	sdramc_wr(config, AT91_SDRAMC_CR, config->cr);

	/* Step#2 For mobile SDRAM, temperature-compensated self refresh(TCSR),... */

	/* Step#3 The SDRAM memory type must be set in the Memory Device Register */
	sdramc_wr(config, AT91_SDRAMC_MDR, config->mdr);

	/* Step#4 The minimum pause of 200 us is provided to precede any single toggle */
	early_udelay(200);

	/* Step#5 A NOP command is issued to the SDRAM devices */
	sdramc_wr(config, AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NOP);
	writel(0x00000000, sdram_address);

	/* Step#6 An All Banks Precharge command is issued to the SDRAM devices  */
	sdramc_wr(config, AT91_SDRAMC_MR, AT91_SDRAMC_MODE_PRECHARGE);
	writel(0x00000000, sdram_address);

	/* Pause cycles */
	early_udelay(2000);

	/* Step#7 Eight auto-refresh cycles are provided */
	for (i = 0; i < 8; i++) {
		sdramc_wr(config, AT91_SDRAMC_MR, AT91_SDRAMC_MODE_REFRESH);
		writel(0x00000001 + i, sdram_address + 4 + 4 * i);
	}

	/* Pause cycles */
	early_udelay(200);

	/* Step#8 A Mode Register set (MRS) cycle is issued to program (TCSR, PASR, DS) */
	sdramc_wr(config, AT91_SDRAMC_MR, AT91_SDRAMC_MODE_LMR);
	writel(0xcafedede, sdram_address + 0x24);

	/*  Pause cycles */
	early_udelay(200);

	/* Step#9 For mobile SDRAM initialization, an Extended Mode Register set ... */

	/* Step#10 The application must go into Normal Mode, setting Mode to 0
	 * and perform a write access at any location in the SDRAM.
	 */
	sdramc_wr(config, AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);	// Set mode
	writel(0x00000000, sdram_address);				// Perform mode

	/* Step#11 Write the refresh rate into the count field in the Refresh Register. */
	sdramc_wr(config, AT91_SDRAMC_TR, config->tr);

	return 0;
}
