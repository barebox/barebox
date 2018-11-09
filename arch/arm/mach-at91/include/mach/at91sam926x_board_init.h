#ifndef __AT91SAM926X_BOARD_INIT_H__
#define __AT91SAM926X_BOARD_INIT_H__
/*
 * Copyright (C) 2008 Ronetix Ilko Iliev (www.ronetix.at)
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <mach/at91sam9_sdramc.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91_rstc.h>
#include <mach/at91_pio.h>
#include <mach/at91_pmc.h>
#include <mach/at91_wdt.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/at91sam926x.h>

struct at91sam926x_board_cfg {
	/* SoC specific */
	void __iomem *pio;
	void __iomem *sdramc;
	u32 ebi_pio_is_peripha;
	void __iomem *matrix_csa;

	/* board specific */
	u32 wdt_mr;
	u32 ebi_pio_pdr;
	u32 ebi_pio_ppudr;
	u32 ebi_csa;
	u32 smc_cs;
	u32 smc_mode;
	u32 smc_cycle;
	u32 smc_pulse;
	u32 smc_setup;
	u32 pmc_mor;
	u32 pmc_pllar;
	u32 pmc_mckr1;
	u32 pmc_mckr2;
	u32 sdrc_cr;
	u32 sdrc_tr1;
	u32 sdrc_mdr;
	u32 sdrc_tr2;
	u32 rstc_rmr;
};


static void __always_inline access_sdram(void)
{
	writel(0x00000000, AT91_CHIPSELECT_1);
}

static void __always_inline pmc_check_mckrdy(void)
{
	u32 r;

	do {
		r = readl(AT91SAM926X_BASE_PMC + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MCKRDY));
}

static int __always_inline running_in_sram(void)
{
	u32 addr = get_pc();

	addr >>= 28;
	return addr == 0;
}

static void __always_inline at91sam926x_sdramc_init(struct at91sam926x_board_cfg *cfg)
{
	u32 r;
	int i;
	int in_sram = running_in_sram();

	/* SDRAMC Check if Refresh Timer Counter is already initialized */
	r = __raw_readl(cfg->sdramc + AT91_SDRAMC_TR);
	if (r && !in_sram)
		return;

	/* SDRAMC_MR : Normal Mode */
	__raw_writel(AT91_SDRAMC_MODE_NORMAL, cfg->sdramc + AT91_SDRAMC_MR);

	/* SDRAMC_TR - Refresh Timer register */
	__raw_writel(cfg->sdrc_tr1, cfg->sdramc + AT91_SDRAMC_TR);

	/* SDRAMC_CR - Configuration register*/
	__raw_writel(cfg->sdrc_cr, cfg->sdramc + AT91_SDRAMC_CR);

	/* Memory Device Type */
	__raw_writel(cfg->sdrc_mdr, cfg->sdramc + AT91_SDRAMC_MDR);

	/* SDRAMC_MR : Precharge All */
	__raw_writel(AT91_SDRAMC_MODE_PRECHARGE, cfg->sdramc + AT91_SDRAMC_MR);
	access_sdram();

	/* SDRAMC_MR : refresh */
	__raw_writel(AT91_SDRAMC_MODE_REFRESH, cfg->sdramc + AT91_SDRAMC_MR);

	/* access SDRAM 8 times */
	for (i = 0; i < 8; i++)
		access_sdram();

	/* SDRAMC_MR : Load Mode Register */
	__raw_writel(AT91_SDRAMC_MODE_LMR, cfg->sdramc + AT91_SDRAMC_MR);
	access_sdram();

	/* SDRAMC_MR : Normal Mode */
	__raw_writel(AT91_SDRAMC_MODE_NORMAL, cfg->sdramc + AT91_SDRAMC_MR);
	access_sdram();

	/* SDRAMC_TR : Refresh Timer Counter */
	__raw_writel(cfg->sdrc_tr2, cfg->sdramc + AT91_SDRAMC_TR);
	access_sdram();
}

static void __always_inline at91sam926x_board_init(void __iomem *smcbase,
						   struct at91sam926x_board_cfg *cfg)
{
	u32 r;
	void __iomem *pmc = IOMEM(AT91SAM926X_BASE_PMC);

	if (!IS_ENABLED(CONFIG_AT91SAM926X_BOARD_INIT))
		return;

	__raw_writel(cfg->wdt_mr, AT91SAM926X_BASE_WDT + AT91_WDT_MR);

	/* configure PIOx as EBI0 D[16-31] */
	at91_mux_gpio_disable(cfg->pio, cfg->ebi_pio_pdr);
	at91_mux_set_pullup(cfg->pio, cfg->ebi_pio_ppudr, true);
	if (cfg->ebi_pio_is_peripha)
		at91_mux_set_A_periph(cfg->pio, cfg->ebi_pio_ppudr);

	writel(cfg->ebi_csa, cfg->matrix_csa);

	/* flash */
	writel(cfg->smc_mode, smcbase + cfg->smc_cs * 0x10 + AT91_SAM9_SMC_MODE);
	writel(cfg->smc_cycle, smcbase + cfg->smc_cs * 0x10 + AT91_SMC_CYCLE);
	writel(cfg->smc_pulse, smcbase + cfg->smc_cs * 0x10 + AT91_SMC_PULSE);
	writel(cfg->smc_setup, smcbase + cfg->smc_cs * 0x10 + AT91_SMC_SETUP);

	/* PMC Check if the PLL is already initialized */
	r = readl(pmc + AT91_PMC_MCKR);
	if ((r & AT91_PMC_CSS) && !running_in_sram())
		return;

	/* Enable the Main Oscillator */
	writel(cfg->pmc_mor, pmc + AT91_CKGR_MOR);
	do {
		r = readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_MOSCS));

	/* PLLAR: x MHz for PCK */
	writel(cfg->pmc_pllar, pmc + AT91_CKGR_PLLAR);
	do {
		r = readl(pmc + AT91_PMC_SR);
	} while (!(r & AT91_PMC_LOCKA));

	/* PCK/x = MCK Master Clock from SLOW */
	writel(cfg->pmc_mckr1, pmc + AT91_PMC_MCKR);
	pmc_check_mckrdy();

	/* PCK/x = MCK Master Clock from PLLA */
	writel(cfg->pmc_mckr2, pmc + AT91_PMC_MCKR);
	pmc_check_mckrdy();

	/* Init SDRAM */
	at91sam926x_sdramc_init(cfg);

	/* User reset enable*/
	writel(cfg->rstc_rmr, AT91SAM926X_BASE_RSTC + AT91_RSTC_MR);

	/*
	 * When boot from external boot
	 * we need to enable mck and ohter clock
	 * so enable all of them
	 * We will shutdown what we don't need later
	 */
	writel(0xffffffff, pmc + AT91_PMC_PCER);
}

#if defined CONFIG_ARCH_AT91SAM9260
#include <mach/at91sam9260.h>
static void __always_inline at91sam9260_board_init(struct at91sam926x_board_cfg *cfg)
{
	at91sam926x_board_init(IOMEM(AT91SAM9260_BASE_SMC), cfg);
}
#endif

#if defined CONFIG_ARCH_AT91SAM9261 || defined CONFIG_ARCH_AT91SAM9G10
#include <mach/at91sam9261.h>
static void __always_inline at91sam9261_board_init(struct at91sam926x_board_cfg *cfg)
{
	at91sam926x_board_init(IOMEM(AT91SAM9261_BASE_SMC), cfg);
}
#endif

#if defined CONFIG_ARCH_AT91SAM9263
#include <mach/at91sam9263.h>
static void __always_inline at91sam9263_board_init(struct at91sam926x_board_cfg *cfg)
{
	at91sam926x_board_init(IOMEM(AT91SAM9263_BASE_SMC0), cfg);
}
#endif

#endif /* __AT91SAM926X_BOARD_INIT_H__ */
