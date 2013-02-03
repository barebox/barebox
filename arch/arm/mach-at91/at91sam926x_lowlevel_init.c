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
#include <mach/at91_lowlevel_init.h>
#include <mach/io.h>
#include <init.h>
#include <sizes.h>

#define __gpio_init inline
#include "gpio.h"

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

#define at91_sdramc_read(field) \
	__raw_readl(cfg->sdramc + field)

#define at91_sdramc_write(field, value) \
	__raw_writel(value, cfg->sdramc + field)

void __bare_init at91sam926x_sdramc_init(struct at91sam926x_lowlevel_cfg *cfg)
{
	u32 r;
	int i;
	int in_sram = running_in_sram();

	/*
	 * SDRAMC Check if Refresh Timer Counter is already initialized
	 */
	r = at91_sdramc_read(AT91_SDRAMC_TR);
	if (r && !in_sram)
		return;

	/* SDRAMC_MR : Normal Mode */
	at91_sdramc_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);

	/* SDRAMC_TR - Refresh Timer register */
	at91_sdramc_write(AT91_SDRAMC_TR, cfg->sdrc_tr1);

	/* SDRAMC_CR - Configuration register*/
	at91_sdramc_write(AT91_SDRAMC_CR, cfg->sdrc_cr);

	/* Memory Device Type */
	at91_sdramc_write(AT91_SDRAMC_MDR, cfg->sdrc_mdr);

	/* SDRAMC_MR : Precharge All */
	at91_sdramc_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_PRECHARGE);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_MR : refresh */
	at91_sdramc_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_REFRESH);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRAMC_MR : Load Mode Register */
	at91_sdramc_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_LMR);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_MR : Normal Mode */
	at91_sdramc_write(AT91_SDRAMC_MR, AT91_SDRAMC_MODE_NORMAL);

	/* access SDRAM */
	access_sdram();

	/* SDRAMC_TR : Refresh Timer Counter */
	at91_sdramc_write(AT91_SDRAMC_TR, cfg->sdrc_tr2);

	/* access SDRAM */
	access_sdram();
}

void __bare_init at91sam926x_lowlevel_init(struct at91sam926x_lowlevel_cfg *cfg)
{
	u32 r;
	int in_sram = running_in_sram();

	at91sam926x_lowlevel_board_config(cfg);

	__raw_writel(cfg->wdt_mr, AT91_BASE_WDT + AT91_WDT_MR);

	/* configure PIOx as EBI0 D[16-31] */
	at91_mux_gpio_disable(cfg->pio, cfg->ebi_pio_pdr);
	at91_mux_set_pullup(cfg->pio, cfg->ebi_pio_ppudr, true);
	if (cfg->ebi_pio_is_peripha)
		at91_mux_set_A_periph(cfg->pio, cfg->ebi_pio_ppudr);

	at91_sys_write(cfg->matrix_csa, cfg->ebi_csa);

	/* flash */
	at91_smc_write(cfg->smc_cs, AT91_SMC_MODE, cfg->smc_mode);

	at91_smc_write(cfg->smc_cs, AT91_SMC_CYCLE, cfg->smc_cycle);

	at91_smc_write(cfg->smc_cs, AT91_SMC_PULSE, cfg->smc_pulse);

	at91_smc_write(cfg->smc_cs, AT91_SMC_SETUP, cfg->smc_setup);

	/*
	 * PMC Check if the PLL is already initialized
	 */
	r = at91_pmc_read(AT91_PMC_MCKR);
	if (r & AT91_PMC_CSS && !in_sram)
		return;

	/*
	 * Enable the Main Oscillator
	 */
	at91_pmc_write(AT91_CKGR_MOR, cfg->pmc_mor);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

	/*
	 * PLLAR: x MHz for PCK
	 */
	at91_pmc_write(AT91_CKGR_PLLAR, cfg->pmc_pllar);

	do {
		r = at91_pmc_read(AT91_PMC_SR);
	} while (!(r & AT91_PMC_LOCKA));

	/*
	 * PCK/x = MCK Master Clock from SLOW
	 */
	at91_pmc_write(AT91_PMC_MCKR, cfg->pmc_mckr1);

	pmc_check_mckrdy();

	/*
	 * PCK/x = MCK Master Clock from PLLA
	 */
	at91_pmc_write(AT91_PMC_MCKR, cfg->pmc_mckr2);

	pmc_check_mckrdy();

	/*
	 * Init SDRAM
	 */
	at91sam926x_sdramc_init(cfg);

	/* User reset enable*/
	at91_sys_write(AT91_RSTC_MR, cfg->rstc_rmr);

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
}
