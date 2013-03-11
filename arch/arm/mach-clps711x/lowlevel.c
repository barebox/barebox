/*
 * Copyright (C) 2012 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <sizes.h>

#include <asm/io.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>

#include <mach/clps711x.h>

void __naked __bare_init clps711x_barebox_entry(u32 pllmult)
{
	u32 cpu, bus;

	/* Check if we running from external 13 MHz clock */
	if (!(readl(SYSFLG2) & SYSFLG2_CKMODE)) {
		/* Setup bus wait state scaling factor to 2  */
		writel(SYSCON3_CLKCTL0 | SYSCON3_CLKCTL1, SYSCON3);
		asm("nop");

		/* Check valid multiplier, default to 74 MHz */
		if ((pllmult < 20) || (pllmult > 50))
			pllmult = 40;

		/* Setup PLL */
		writel(pllmult << 24, PLLW);
		asm("nop");

		/* Check for old CPUs without PLL */
		if ((readl(PLLR) >> 24) != pllmult)
			cpu = 73728000;
		else
			cpu = pllmult * 3686400;

		if (cpu >= 36864000)
			bus = cpu / 2;
		else
			bus = 36864000 / 2;
	} else {
		bus = 13000000;
		/* Setup bus wait state scaling factor to 1  */
		writel(0, SYSCON3);
		asm("nop");
	}

	/* CLKEN select, SDRAM width=32 */
	writel(SYSCON2_CLKENSL, SYSCON2);

	/* Setup SDRAM params (64MB, 16Bit*2, CAS=3) */
	writel(SDCONF_CASLAT_3 | SDCONF_SIZE_256 | SDCONF_WIDTH_16 |
	       SDCONF_CLKCTL | SDCONF_ACTIVE, SDCONF);

	/* Setup Refresh Rate (64ms 8K Blocks) */
	writel((64 * bus) / (8192 * 1000), SDRFPR);

	/* Disable UART, IrDa, LCD */
	writel(0, SYSCON1);
	/* Disable PWM */
	writew(0, PMPCON);
	/* Disable LED flasher */
	writew(0, LEDFLSH);

	barebox_arm_entry(SDRAM0_BASE, SZ_8M, 0);
}
