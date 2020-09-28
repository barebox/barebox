// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2006, Atmel Corporation
 * Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries
 *
 * Atmel/Microchip's name may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 */

#define pr_fmt(fmt) "at91pmc: " fmt

#include <common.h>
#include <mach/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/at91_pmc_ll.h>
#include <mach/early_udelay.h>

#define SFR_UTMICKTRIM	0x30	/* UTMI Clock Trimming Register */
#define AT91_UTMICKTRIM_FREQ	0x03

#define PMC_GCSR0	0xC0	/* PMCv2 Generic Clock Status Register 0 */
#define PMC_GCSR1	0xC4	/* PMCv2 Generic Clock Status Register 1 */

#define at91_pmc_write(off, val) writel(val, pmc_base + off)
#define at91_pmc_read(off) readl(pmc_base + off)

#define MHZ (1000 * 1000UL)

static unsigned long at91_pmc_get_main_xtal(void __iomem *pmc_base)
{
	u32 tmp;

	/* Enable a measurement of the Main Crystal Oscillator */
	tmp = at91_pmc_read(AT91_CKGR_MCFR);
	tmp |= AT91_PMC_CCSS_XTAL_OSC;
	tmp |= AT91_PMC_RCMEAS;
	at91_pmc_write(AT91_CKGR_MCFR, tmp);

	do {
		tmp = at91_pmc_read(AT91_CKGR_MCFR);
	} while (!(tmp & AT91_PMC_MAINRDY));

	/* read once more like the datasheet says */
	tmp = at91_pmc_read(AT91_CKGR_MCFR) & AT91_PMC_MAINF;

	return tmp * (AT91_SLOW_CLOCK / 16);
}

void at91_pmc_init(void __iomem *pmc_base, unsigned int flags)
{
	u32 tmp;

	/*
	 * Switch the master clock to the slow clock without modifying other
	 * parameters. It is assumed that ROM code set H32MXDIV, PLLADIV2,
	 * PCK_DIV3.
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~AT91_PMC_ALT_PCKR_CSS;
	tmp |= AT91_PMC_CSS_SLOW;
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MCKRDY))
		;

	if (flags & AT91_PMC_LL_FLAG_SAM9X5_PMC) {
		/*
		 * Enable the Main Crystal Oscillator
		 * tST_max = 2ms
		 * Startup Time: 32768 * 2ms / 8 = 8
		 */
		tmp = at91_pmc_read(AT91_CKGR_MOR);
		tmp &= ~AT91_PMC_OSCOUNT;
		tmp &= ~AT91_PMC_KEY_MASK;
		tmp |= AT91_PMC_MOSCEN;
		tmp |= AT91_PMC_OSCOUNT_(8);
		tmp |= AT91_PMC_KEY;
		at91_pmc_write(AT91_CKGR_MOR, tmp);

		while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MOSCS))
			;

		if (flags & AT91_PMC_LL_FLAG_MEASURE_XTAL)
			(void)at91_pmc_get_main_xtal(pmc_base);

		/* Switch from internal 12MHz RC to the Main Crystal Oscillator */
		tmp = at91_pmc_read(AT91_CKGR_MOR);
		tmp &= ~AT91_PMC_OSCBYPASS;
		tmp &= ~AT91_PMC_KEY_MASK;
		tmp |= AT91_PMC_KEY;
		if (flags & AT91_PMC_LL_FLAG_MCK_BYPASS)
			tmp |= AT91_PMC_OSCBYPASS;
		at91_pmc_write(AT91_CKGR_MOR, tmp);

		tmp = at91_pmc_read(AT91_CKGR_MOR);
		tmp |= AT91_PMC_MOSCSEL;
		tmp &= ~AT91_PMC_KEY_MASK;
		tmp |= AT91_PMC_KEY;
		at91_pmc_write(AT91_CKGR_MOR, tmp);

		while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MOSCSELS))
			;

		if (flags & AT91_PMC_LL_FLAG_DISABLE_RC) {
			/* Disable the 12MHz RC Oscillator */
			tmp = at91_pmc_read(AT91_CKGR_MOR);
			tmp &= ~AT91_PMC_MOSCRCEN;
			tmp &= ~AT91_PMC_KEY_MASK;
			tmp |= AT91_PMC_KEY;
			at91_pmc_write(AT91_CKGR_MOR, tmp);
		}

	} else {
		/*
		 * Enable the Main Crystal Oscillator
		 * tST_max = 2ms
		 * Startup Time: 32768 * 2ms / 8 = 8
		 */
		tmp = at91_pmc_read(AT91_CKGR_MOR);
		tmp &= ~AT91_PMC_OSCOUNT;
		tmp |= AT91_PMC_MOSCEN;
		tmp |= AT91_PMC_OSCOUNT_(8);
		at91_pmc_write(AT91_CKGR_MOR, tmp);

		while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MOSCS))
			;
	}

	/* After stablization, switch to Main Clock */
	if ((at91_pmc_read(AT91_PMC_MCKR) & AT91_PMC_ALT_PCKR_CSS) == AT91_PMC_CSS_SLOW) {
		tmp = at91_pmc_read(AT91_PMC_MCKR);
		tmp &= ~(0x1 << 13);
		tmp &= ~AT91_PMC_ALT_PCKR_CSS;
		tmp |= AT91_PMC_CSS_MAIN;
		at91_pmc_write(AT91_PMC_MCKR, tmp);

		while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MCKRDY))
			;

		tmp &= ~AT91_PMC_PRES;
		tmp |= AT91_PMC_PRES_1;
		at91_pmc_write(AT91_PMC_MCKR, tmp);

		while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MCKRDY))
			;
	}
}

void at91_pmc_cfg_plla(void __iomem *pmc_base, u32 pmc_pllar,
		       unsigned int __always_unused flags)
{
	/* Always disable PLL before configuring it */
	at91_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA);
	at91_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA | pmc_pllar);

	while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_LOCKA))
		;
}

void at91_pmc_cfg_mck(void __iomem *pmc_base, u32 pmc_mckr, unsigned int flags)
{
	u32 tmp;

	/*
	 * Program the PRES field in the AT91_PMC_MCKR register
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~(0x1 << 13);

	if (flags & AT91_PMC_LL_FLAG_SAM9X5_PMC) {
		tmp &= ~AT91_PMC_ALT_PRES;
		tmp |= pmc_mckr & AT91_PMC_ALT_PRES;
	} else {
		tmp &= ~AT91_PMC_PRES;
		tmp |= pmc_mckr & AT91_PMC_PRES;
	}
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	/*
	 * Program the MDIV field in the AT91_PMC_MCKR register
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~AT91_PMC_MDIV;
	tmp |= pmc_mckr & AT91_PMC_MDIV;
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	/*
	 * Program the PLLADIV2 field in the AT91_PMC_MCKR register
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~AT91_PMC_PLLADIV2;
	tmp |= pmc_mckr & AT91_PMC_PLLADIV2;
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	/*
	 * Program the H32MXDIV field in the AT91_PMC_MCKR register
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~AT91_PMC_H32MXDIV;
	tmp |= pmc_mckr & AT91_PMC_H32MXDIV;
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	/*
	 * Program the CSS field in the AT91_PMC_MCKR register,
	 * wait for MCKRDY bit to be set in the PMC_SR register
	 */
	tmp = at91_pmc_read(AT91_PMC_MCKR);
	tmp &= ~AT91_PMC_ALT_PCKR_CSS;
	tmp |= pmc_mckr & AT91_PMC_ALT_PCKR_CSS;
	at91_pmc_write(AT91_PMC_MCKR, tmp);

	while (!(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_MCKRDY))
		;
}

static void pmc_configure_utmi_ref_clk(void __iomem *pmc_base,
				       void __iomem *sfr_base,
				       unsigned long main_xtal)
{
	unsigned int utmi_ref_clk_freq = 0, tmp;

	/*
	 * If mainck rate is different from 12 MHz, we have to configure
	 * the FREQ field of the SFR_UTMICKTRIM register to generate properly
	 * the utmi clock.
	 */
	if (main_xtal < (16 +  4) * MHZ)
		utmi_ref_clk_freq++;
	if (main_xtal < (24 + 10) * MHZ)
		utmi_ref_clk_freq++;
	if (main_xtal < (48 + 10) * MHZ)
		utmi_ref_clk_freq++;

	/*
	 * Not supported on SAMA5D2 but it's not an issue since MAINCK
	 * maximum value is 24 MHz.
	 */
	tmp = readl(sfr_base + SFR_UTMICKTRIM);
	tmp &= ~AT91_UTMICKTRIM_FREQ;
	tmp |= utmi_ref_clk_freq;
	writel(tmp, sfr_base + SFR_UTMICKTRIM);
}

static void pmc_uckr_clk(void __iomem *pmc_base,
			 void __iomem *sfr_base,
			 unsigned long main_xtal)
{
	unsigned int uckr = at91_pmc_read(AT91_CKGR_UCKR);
	unsigned int sr;

	if (main_xtal) {
		pmc_configure_utmi_ref_clk(pmc_base, sfr_base,
						 main_xtal);
		uckr |= (AT91_PMC_UPLLCOUNT_DEFAULT |
			 AT91_PMC_UPLLEN | AT91_PMC_BIASEN);
		sr = AT91_PMC_LOCKU;
	} else {
		uckr &= ~(AT91_PMC_UPLLEN | AT91_PMC_BIASEN);
		sr = 0;
	}

	at91_pmc_write(AT91_CKGR_UCKR, uckr);

	do {
		early_udelay(1);
	} while ((at91_pmc_read(AT91_PMC_SR) & AT91_PMC_LOCKU) != sr);
}

static inline unsigned gck_status(unsigned periph_id,
				  unsigned flags)
{
	if (flags & AT91_PMC_LL_FLAG_GCSR)
		return periph_id < 32 ? PMC_GCSR0 : PMC_GCSR1;

	return AT91_PMC_SR;
}

static inline unsigned gck_ready(unsigned status,
				 unsigned periph_id,
				 unsigned flags)
{
	unsigned mask;

	if (flags & AT91_PMC_LL_FLAG_GCSR)
		mask = 1 << (periph_id & 0x1f);
	else
		mask = AT91_PMC_GCKRDY;

	return status & mask;
}

int at91_pmc_enable_generic_clock(void __iomem *pmc_base,
				  void __iomem *sfr_base,
				  unsigned int periph_id,
				  unsigned int clk_source, unsigned int div,
				  unsigned int flags)
{
	unsigned long main_xtal;
	unsigned int regval, status;
	unsigned int timeout = 1000;

	if (periph_id > 0x7f)
		return -EINVAL;

	if (div > 0xff)
		return -EINVAL;

	main_xtal = at91_pmc_get_main_xtal(pmc_base);

	if ((flags & AT91_PMC_LL_FLAG_PMC_UTMI) &&
	    !(at91_pmc_read(AT91_PMC_SR) & AT91_PMC_LOCKU))
		pmc_uckr_clk(pmc_base, sfr_base, main_xtal);

	at91_pmc_write(AT91_PMC_PCR, periph_id);
	regval = at91_pmc_read(AT91_PMC_PCR);
	regval &= ~AT91_PMC_GCKCSS;
	regval &= ~AT91_PMC_GCKDIV;

	regval |= clk_source;
	regval |= AT91_PMC_PCR_CMD | AT91_PMC_GCKDIV_(div) | AT91_PMC_GCK_EN;

	at91_pmc_write(AT91_PMC_PCR, regval);

	for (timeout = 1000; timeout; timeout--) {
		early_udelay(1);

		status = at91_pmc_read(gck_status(periph_id, flags));
		if (gck_ready(status, periph_id, flags))
			return 0;
	}

	pr_warn("Timeout waiting for GCK ready!\n");

	return 0;
}
