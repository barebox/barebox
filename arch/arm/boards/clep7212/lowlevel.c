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

#define MAIN_CLOCK		3686400
#define CPU_SPEED		92160000
#define BUS_SPEED		(CPU_SPEED / 2)

#define PLL_VALUE		(((CPU_SPEED * 2) / MAIN_CLOCK) << 24)
#define SDRAM_REFRESH_RATE	(64 * (BUS_SPEED / (8192 * 1000)))

void __naked __bare_init barebox_arm_reset_vector(void)
{
	u32 tmp;

	arm_cpu_lowlevel_init();

	/* Setup base clock */
	writel(SYSCON3_CLKCTL0 | SYSCON3_CLKCTL1, SYSCON3);
	asm("nop");

	/* Setup PLL */
	writel(PLL_VALUE, PLLW);
	asm("nop");

	/* CLKEN select, SDRAM width=32 */
	writel(SYSCON2_CLKENSL, SYSCON2);

	/* Enable SDQM pins */
	tmp = readl(SYSCON3);
	tmp &= ~SYSCON3_ENPD67;
	writel(tmp, SYSCON3);

	/* Setup Refresh Rate (64ms 8K Blocks) */
	writel(SDRAM_REFRESH_RATE, SDRFPR);

	/* Setup SDRAM (32MB, 16Bit*2, CAS=3) */
	writel(SDCONF_CASLAT_3 | SDCONF_SIZE_256 | SDCONF_WIDTH_16 |
	       SDCONF_CLKCTL | SDCONF_ACTIVE, SDCONF);

	barebox_arm_entry(SDRAM0_BASE, SZ_32M, 0);
}
