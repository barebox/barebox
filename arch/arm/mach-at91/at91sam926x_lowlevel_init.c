/*
 * Copyright (C) 2008 Ronetix Ilko Iliev (www.ronetix.at)
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <mach/io.h>
#include <init.h>

static void inline access_sdram(void)
{
	writel(0x00000000, AT91_SDRAM_BASE);
}

static void inline pmc_check_mckrdy(void)
{
	u32 r;

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_MCKRDY));
}

static int inline running_in_sram(void)
{
	u32 addr = get_pc();

	addr >>= 28;
	return addr == 0;
}

void __naked __bare_init reset(void)
{
	u32 r;
	int i;
	int in_sram = running_in_sram();

	common_reset();

	__raw_writel(CONFIG_SYS_WDTC_WDMR_VAL, AT91_BASE_WDT + AT91_WDT_MR);

	/* configure PIOx as EBI0 D[16-31] */
#ifdef CONFIG_ARCH_AT91SAM9263
	__raw_writel(CONFIG_SYS_PIOD_PDR_VAL1, AT91_BASE_PIOD + PIO_PDR);
	__raw_writel(CONFIG_SYS_PIOD_PPUDR_VAL, AT91_BASE_PIOD + PIO_PUDR);
	__raw_writel(CONFIG_SYS_PIOD_PPUDR_VAL, AT91_BASE_PIOD + PIO_ASR);
#else
	__raw_writel(CONFIG_SYS_PIOC_PDR_VAL1, AT91_BASE_PIOC + PIO_PDR);
	__raw_writel(CONFIG_SYS_PIOC_PPUDR_VAL, AT91_BASE_PIOC + PIO_PUDR);
#endif

#if defined(AT91_MATRIX_EBI0CSA)
	at91_sys_write(AT91_MATRIX_EBI0CSA, CONFIG_SYS_MATRIX_EBI0CSA_VAL);
#else /* AT91_MATRIX_EBICSA */
	at91_sys_write(AT91_MATRIX_EBICSA, CONFIG_SYS_MATRIX_EBICSA_VAL);
#endif

	/* flash */
	at91_smc_write(CONFIG_SYS_SMC_CS, AT91_SMC_MODE, CONFIG_SYS_SMC_MODE_VAL);

	at91_smc_write(CONFIG_SYS_SMC_CS, AT91_SMC_CYCLE, CONFIG_SYS_SMC_CYCLE_VAL);

	at91_smc_write(CONFIG_SYS_SMC_CS, AT91_SMC_PULSE, CONFIG_SYS_SMC_PULSE_VAL);

	at91_smc_write(CONFIG_SYS_SMC_CS, AT91_SMC_SETUP, CONFIG_SYS_SMC_SETUP_VAL);

	/*
	 * PMC Check if the PLL is already initialized
	 */
	r = at91_pmc_read(AT91_PMC_MCKR);
	if (r & AT91_PMC_CSS && !in_sram)
		goto end;

	/*
	 * Enable the Main Oscillator
	 */
	at91_pmc_write(AT91_CKGR_MOR, CONFIG_SYS_MOR_VAL);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

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
	at91_pmc_write(AT91_PMC_MCKR, CONFIG_SYS_MCKR1_VAL);

	pmc_check_mckrdy();

	/*
	 * PCK/x = MCK Master Clock from PLLA
	 */
	at91_pmc_write(AT91_PMC_MCKR, CONFIG_SYS_MCKR2_VAL);

	pmc_check_mckrdy();

	/*
	 * Init SDRAM
	 */

	/*
	 * SDRAMC Check if Refresh Timer Counter is already initialized
	 */
	r = at91_sys_read(AT91_SDRAMC_TR);
	if (r && !in_sram)
		goto end;

	/* SDRAMC_MR : Normal Mode */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);

	/* SDRAMC_TR - Refresh Timer register */
	at91_sys_write(AT91_SDRAMC_TR, CONFIG_SYS_SDRC_TR_VAL1);

	/* SDRAMC_CR - Configuration register*/
	at91_sys_write(AT91_SDRAMC_CR, CONFIG_SYS_SDRC_CR_VAL);

	/* Memory Device Type */
	at91_sys_write(AT91_SDRAMC_MDR, CONFIG_SYS_SDRC_MDR_VAL);

	/* SDRAMC_MR : Precharge All */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_PRECHARGE);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_MR : refresh */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_REFRESH);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRAMC_MR : Load Mode Register */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_LMR);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_MR : Normal Mode */
	at91_sys_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_TR : Refresh Timer Counter */
	at91_sys_write(AT91_SDRAMC_TR, CONFIG_SYS_SDRC_TR_VAL2);

	/* access SDRAM */
	access_sdram();

	/* User reset enable*/
	at91_sys_write(AT91_RSTC_MR, CONFIG_SYS_RSTC_RMR_VAL);

#ifdef CONFIG_SYS_MATRIX_MCFG_REMAP
	/* MATRIX_MCFG - REMAP all masters */
	at91_sys_write(AT91_MATRIX_MCFG0, 0x1FF);
#endif
	/*
	 * When boot from external boot
	 * we need to enable mck and ohter clock
	 * so enable all of them
	 * We will shutdown what we don't need later
	 */
	at91_pmc_write(AT91_PMC_PCER, 0xffffffff);

end:
	board_init_lowlevel_return();
}
