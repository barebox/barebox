// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2017, Microchip Corporation
 *
 * Microchip's name may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 */

#include <mach/sama5d2_ll.h>
#include <mach/at91_ddrsdrc.h>
#include <mach/ddramc.h>
#include <mach/early_udelay.h>
#include <mach/at91_rstc.h>
#include <asm/barebox-arm.h>

#define sama5d2_pmc_write(off, val) writel(val, SAMA5D2_BASE_PMC + off)
#define sama5d2_pmc_read(off) readl(SAMA5D2_BASE_PMC + off)

void sama5d2_ddr2_init(struct at91_ddramc_register *ddramc_reg_config)
{
	unsigned int reg;

	/* enable ddr2 clock */
	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_MPDDRC);
	sama5d2_pmc_write(AT91_PMC_SCER, AT91CAP9_PMC_DDR);

	reg = AT91_MPDDRC_RD_DATA_PATH_ONE_CYCLES;
	writel(reg, SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_RD_DATA_PATH);

	reg = readl(SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_IO_CALIBR);
	reg &= ~AT91_MPDDRC_RDIV;
	reg &= ~AT91_MPDDRC_TZQIO;
	reg |= AT91_MPDDRC_RDIV_DDR2_RZQ_50;
	reg |= AT91_MPDDRC_TZQIO_(101);
	writel(reg, SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_IO_CALIBR);

	/* DDRAM2 Controller initialize */
	at91_ddram_initialize(SAMA5D2_BASE_MPDDRC, IOMEM(SAMA5_DDRCS),
			      ddramc_reg_config);
}

static void sama5d2_pmc_init(void)
{
	at91_pmc_init(SAMA5D2_BASE_PMC, AT91_PMC_LL_SAMA5D2);

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	sama5d2_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA);
	sama5d2_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA
		       | AT91_PMC3_MUL_(40) | AT91_PMC_OUT_0
		       | AT91_PMC_PLLCOUNT
		       | AT91_PMC_DIV_BYPASS);

	while (!(sama5d2_pmc_read(AT91_PMC_SR) & AT91_PMC_LOCKA))
		;

	/* Initialize PLLA charge pump */
	/* No need: we keep what is set in ROM code */
	//sama5d2_pmc_write(AT91_PMC_PLLICPR, AT91_PMC_IPLLA_3);

	/* Switch PCK/MCK on PLLA output */
	at91_pmc_cfg_mck(SAMA5D2_BASE_PMC,
			AT91_PMC_H32MXDIV
			| AT91_PMC_PLLADIV2_ON
			| AT91SAM9_PMC_MDIV_3
			| AT91_PMC_CSS_PLLA,
			AT91_PMC_LL_SAMA5D2);
}

static void sama5d2_rstc_init(void)
{
	writel(AT91_RSTC_KEY | AT91_RSTC_URSTEN,
	       SAMA5D2_BASE_RSTC + AT91_RSTC_MR);
}

void sama5d2_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();
	sama5d2_pmc_init();
	sama5d2_rstc_init();
}
