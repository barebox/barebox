// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2006, Atmel Corporation
 *
 * Atmel's name may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 */

#include <common.h>
#include <mach/at91_pmc_ll.h>

#define at91_pmc_write(off, val) writel(val, pmc_base + off)
#define at91_pmc_read(off) readl(pmc_base + off)

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

		if (flags & AT91_PMC_LL_FLAG_MEASURE_XTAL) {
			/* Enable a measurement of the Main Crystal Oscillator */
			tmp = at91_pmc_read(AT91_CKGR_MCFR);
			tmp |= AT91_PMC_CCSS_XTAL_OSC;
			tmp |= AT91_PMC_RCMEAS;
			at91_pmc_write(AT91_CKGR_MCFR, tmp);

			while (!(at91_pmc_read(AT91_CKGR_MCFR) & AT91_PMC_MAINRDY))
				;
		}

		/* Switch from internal 12MHz RC to the Main Crystal Oscillator */
		tmp = at91_pmc_read(AT91_CKGR_MOR);
		tmp &= ~AT91_PMC_OSCBYPASS;
		tmp &= ~AT91_PMC_KEY_MASK;
		tmp |= AT91_PMC_KEY;
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
