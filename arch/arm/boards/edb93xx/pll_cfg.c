// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
// SPDX-FileCopyrightText: 2006 Dominic Rath <Dominic.Rath@gmx.de>

/* PLL setup for Cirrus edb93xx boards */

#include <common.h>
#include <io.h>
#include "pll_cfg.h"
#include "early_udelay.h"

/* Called from assembly */
void pll_cfg(void);

void pll_cfg(void)
{
	struct syscon_regs *syscon = (struct syscon_regs *)SYSCON_BASE;

	/* setup PLL1 */
	writel(CLKSET1_VAL, &syscon->clkset1);

	/*
	 * flush the pipeline
	 * writing to CLKSET1 causes the EP93xx to enter standby for between
	 * 8 ms to 16 ms, until PLL1 stabilizes
	 */
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

	/* setup PLL2 */
	writel(CLKSET2_VAL, &syscon->clkset2);

	/*
	 * the user's guide recommends to wait at least 1 ms for PLL2 to
	 * stabilize
	 */
	early_udelay(1000);
}
