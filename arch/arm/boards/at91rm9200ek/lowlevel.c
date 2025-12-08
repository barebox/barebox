/*
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <mach/at91/barebox-arm.h>

#include <mach/at91/at91rm9200_mc.h>
#include <mach/at91/at91rm9200.h>
#include <mach/at91/at91_pio.h>
#include <mach/at91/at91_pmc.h>
#include <mach/at91/hardware.h>

#include "board.h"

void static inline access_sdram(void)
{
	writel(0x00000000, AT91_CHIPSELECT_1);
}

AT91_ENTRY_FUNCTION(start_at91rm9200ek, r0, r1, r2)
{
	u32 r;
	int i;
	void __iomem *mc = IOMEM(AT91RM9200_BASE_MC);
	void __iomem *pmc = IOMEM(AT91RM9200_BASE_PMC);

	arm_cpu_lowlevel_init();

	/*
	 * PMC Check if the PLL is already initialized
	 */
	r = readl(pmc + AT91_PMC_MCKR);
	if (r & AT91_PMC_CSS)
		goto end;

	/*
	 * Enable the Main Oscillator
	 */
	writel(CONFIG_SYS_MOR_VAL, pmc + AT91_CKGR_MOR);

	do {
		r = readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

	/*
	 * EBI_CFGR
	 */
	writel(CONFIG_SYS_EBI_CFGR_VAL, mc + AT91RM9200_EBI_CFGR);

	/*
	 * SMC2_CSR[0]: 16bit, 2 TDF, 4 WS
	 */
	writel(CONFIG_SYS_SMC_CSR0_VAL, mc + AT91RM9200_SMC_CSR(0));

	/*
	 * Init Clocks
	 */

	/*
	 * PLLAR: x MHz for PCK
	 */
	writel(CONFIG_SYS_PLLAR_VAL, pmc + AT91_CKGR_PLLAR);

	do {
		r = readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_LOCKA));

	/*
	 * PCK/x = MCK Master Clock from SLOW
	 */
	writel(CONFIG_SYS_MCKR2_VAL1, pmc + AT91_PMC_MCKR);

	/*
	 * PCK/x = MCK Master Clock from PLLA
	 */
	writel(CONFIG_SYS_MCKR2_VAL2, pmc + AT91_PMC_MCKR);

	do {
		r = readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MCKRDY));

	/*
	 * Init SDRAM
	 */

	/* PIOC_ASR: Configure PIOC as peripheral (D16/D31) */
	writel(CONFIG_SYS_PIOC_ASR_VAL, AT91RM9200_BASE_PIOC + PIO_ASR);
	/* PIOC_BSR */
	writel(CONFIG_SYS_PIOC_BSR_VAL, AT91RM9200_BASE_PIOC + PIO_BSR);
	/* PIOC_PDR */
	writel(CONFIG_SYS_PIOC_PDR_VAL, AT91RM9200_BASE_PIOC + PIO_PDR);

	/* EBI_CSA : CS1=SDRAM */
	writel(CONFIG_SYS_EBI_CSA_VAL, mc + AT91RM9200_EBI_CSA);

	/* SDRC_CR */
	writel(CONFIG_SYS_SDRC_CR_VAL, mc + AT91RM9200_SDRAMC_CR);
	/* SDRC_MR : Precharge All */
	writel(AT91RM9200_SDRAMC_MODE_PRECHARGE, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : refresh */
	writel(AT91RM9200_SDRAMC_MODE_REFRESH, mc + AT91RM9200_SDRAMC_MR);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRC_MR : Load Mode Register */
	writel(AT91RM9200_SDRAMC_MODE_LMR, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_TR : Write refresh rate */
	writel(CONFIG_SYS_SDRC_TR_VAL, mc + AT91RM9200_SDRAMC_TR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : Normal Mode */
	writel(AT91RM9200_SDRAMC_MODE_NORMAL, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();

	/* switch from FastBus to Asynchronous clock mode */
	r = get_cr();
	r |= 0xC0000000;	/* set bit 31 (iA) and 30 (nF) */
	set_cr(r);

end:
	barebox_arm_entry(AT91_CHIPSELECT_1, at91rm9200_get_sdram_size(), NULL);
}
