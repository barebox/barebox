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
	writel(0x00000000, AT91_SDRAM_BASE);
}

void __naked __bare_init barebox_arm_reset_vector(void)
{
	u32 r;
	int i;

	arm_cpu_lowlevel_init();

	/*
	 * PMC Check if the PLL is already initialized
	 */
	r = at91_pmc_read(AT91_PMC_MCKR);
	if (r & AT91_PMC_CSS)
		goto end;

	/*
	 * Enable the Main Oscillator
	 */
	at91_pmc_write(AT91_CKGR_MOR, CONFIG_SYS_MOR_VAL);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

	/*
	 * EBI_CFGR
	 */
	at91_sys_write(AT91_EBI_CFGR, CONFIG_SYS_EBI_CFGR_VAL);

	/*
	 * SMC2_CSR[0]: 16bit, 2 TDF, 4 WS
	 */
	at91_sys_write(AT91_SMC_CSR(0), CONFIG_SYS_SMC_CSR0_VAL);

	/*
	 * Init Clocks
	 */

	/*
	 * PLLAR: x MHz for PCK
	 */
	at91_pmc_write(AT91_CKGR_PLLAR, CONFIG_SYS_PLLAR_VAL);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_LOCKA));

	/*
	 * PCK/x = MCK Master Clock from SLOW
	 */
	at91_pmc_write(AT91_PMC_MCKR, CONFIG_SYS_MCKR2_VAL1);

	/*
	 * PCK/x = MCK Master Clock from PLLA
	 */
	at91_pmc_write(AT91_PMC_MCKR, CONFIG_SYS_MCKR2_VAL2);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_MCKRDY));

	/*
	 * Init SDRAM
	 */

	/* PIOC_ASR: Configure PIOC as peripheral (D16/D31) */
	__raw_writel(CONFIG_SYS_PIOC_ASR_VAL, AT91_BASE_PIOC + PIO_ASR);
	/* PIOC_BSR */
	__raw_writel(CONFIG_SYS_PIOC_BSR_VAL, AT91_BASE_PIOC + PIO_BSR);
	/* PIOC_PDR */
	__raw_writel(CONFIG_SYS_PIOC_PDR_VAL, AT91_BASE_PIOC + PIO_PDR);

	/* EBI_CSA : CS1=SDRAM */
	at91_sys_write(AT91_EBI_CSA, CONFIG_SYS_EBI_CSA_VAL);

	/* SDRC_CR */
	at91_sys_write(AT91_SDRAMC_CR, CONFIG_SYS_SDRC_CR_VAL);
	/* SDRC_MR : Precharge All */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_PRECHARGE);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : refresh */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_REFRESH);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRC_MR : Load Mode Register */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_LMR);
	/* access SDRAM */
	access_sdram();
	/* SDRC_TR : Write refresh rate */
	at91_sys_write(AT91_SDRAMC_TR, CONFIG_SYS_SDRC_TR_VAL);
	/* access SDRAM */
	access_sdram();
	/* SDRC_MR : Normal Mode */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);
	/* access SDRAM */
	access_sdram();

	/* switch from FastBus to Asynchronous clock mode */
	r = get_cr();
	r |= 0xC0000000;	/* set bit 31 (iA) and 30 (nF) */
	set_cr(r);

end:
	barebox_arm_entry(AT91_CHIPSELECT_1, at91rm9200_get_sdram_size(), NULL);
}
