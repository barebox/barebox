/*
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

#include <mach/at91rm9200_mc.h>
#include <mach/at91rm9200.h>
#include <mach/at91_pio.h>
#include <mach/at91_pmc.h>
#include <mach/hardware.h>

void static inline access_sdram(void)
{
	writel(0x00000000, AT91_CHIPSELECT_1);
}

void __naked __bare_init barebox_arm_reset_vector(void)
{
	u32 r;
	int i;
	void __iomem *mc = IOMEM(AT91RM9200_BASE_MC);
	void __iomem *pmc = IOMEM(AT91RM9200_BASE_PMC);

	arm_cpu_lowlevel_init();

	/*
	 * PMC Check if the PLL is already initialized
	 */
	r = __raw_readl(pmc + AT91_PMC_MCKR);
	if (r & AT91_PMC_CSS)
		goto end;

	/*
	 * Enable the Main Oscillator
	 */
	__raw_writel(CONFIG_SYS_MOR_VAL, pmc + AT91_CKGR_MOR);

	do {
		r = __raw_readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

	/*
	 * EBI_CFGR
	 */
	__raw_writel(CONFIG_SYS_EBI_CFGR_VAL, mc + AT91RM9200_EBI_CFGR);

	/*
	 * SMC2_CSR[0]: 16bit, 2 TDF, 4 WS
	 */
	__raw_writel(CONFIG_SYS_SMC_CSR0_VAL, mc + AT91RM9200_SMC_CSR(0));

	/*
	 * Init Clocks
	 */

	/*
	 * PLLAR: x MHz for PCK
	 */
	__raw_writel(CONFIG_SYS_PLLAR_VAL, pmc + AT91_CKGR_PLLAR);

	do {
		r = __raw_readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_LOCKA));

	/*
	 * PCK/x = MCK Master Clock from SLOW
	 */
	__raw_writel(CONFIG_SYS_MCKR2_VAL1, pmc + AT91_PMC_MCKR);

	/*
	 * PCK/x = MCK Master Clock from PLLA
	 */
	__raw_writel(CONFIG_SYS_MCKR2_VAL2, pmc + AT91_PMC_MCKR);

	do {
		r = __raw_readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MCKRDY));

	/*
	 * Init SDRAM
	 */

	/* PIOC_ASR: Configure PIOC as peripheral (D16/D31) */
	__raw_writel(CONFIG_SYS_PIOC_ASR_VAL, AT91RM9200_BASE_PIOC + PIO_ASR);
	/* PIOC_BSR */
	__raw_writel(CONFIG_SYS_PIOC_BSR_VAL, AT91RM9200_BASE_PIOC + PIO_BSR);
	/* PIOC_PDR */
	__raw_writel(CONFIG_SYS_PIOC_PDR_VAL, AT91RM9200_BASE_PIOC + PIO_PDR);

	/* EBI_CSA : CS1=SDRAM */
	__raw_writel(CONFIG_SYS_EBI_CSA_VAL, mc + AT91RM9200_EBI_CSA);

	/* SDRC_CR */
	__raw_writel(CONFIG_SYS_SDRC_CR_VAL, mc + AT91RM9200_SDRAMC_CR);
	/* SDRC_MR : Precharge All */
	__raw_writel(AT91RM9200_SDRAMC_MODE_PRECHARGE, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : refresh */
	__raw_writel(AT91RM9200_SDRAMC_MODE_REFRESH, mc + AT91RM9200_SDRAMC_MR);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRC_MR : Load Mode Register */
	__raw_writel(AT91RM9200_SDRAMC_MODE_LMR, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_TR : Write refresh rate */
	__raw_writel(CONFIG_SYS_SDRC_TR_VAL, mc + AT91RM9200_SDRAMC_TR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : Normal Mode */
	__raw_writel(AT91RM9200_SDRAMC_MODE_NORMAL, mc + AT91RM9200_SDRAMC_MR);
	/* access SDRAM */
	access_sdram();

	/* switch from FastBus to Asynchronous clock mode */
	r = get_cr();
	r |= 0xC0000000;	/* set bit 31 (iA) and 30 (nF) */
	set_cr(r);

end:
	barebox_arm_entry(AT91_CHIPSELECT_1, at91rm9200_get_sdram_size(), NULL);
}
