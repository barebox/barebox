/*
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __AT91_LOWLEVEL_INIT_H__
#define __AT91_LOWLEVEL_INIT_H__

struct at91sam926x_lowlevel_cfg {
	/* SoC specific */
	void __iomem *pio;
	void __iomem *sdramc;
	u32 ebi_pio_is_peripha;
	u32 matrix_csa;

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

#ifdef CONFIG_HAVE_AT91_LOWLEVEL_INIT
void at91sam926x_lowlevel_board_config(struct at91sam926x_lowlevel_cfg *cfg);
void at91sam926x_lowlevel_init(struct at91sam926x_lowlevel_cfg *cfg);
#else
static inline void at91sam926x_lowlevel_board_config(struct at91sam926x_lowlevel_cfg *cfg) {}
static inline void at91sam926x_lowlevel_init(struct at91sam926x_lowlevel_cfg *cfg) {}
#endif

#endif /* __AT91_LOWLEVEL_INIT_H__ */
