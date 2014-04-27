/*
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * Based on Linux clk driver:
 *  Copyright (c) 2014 MundoReader S.L.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>
#include <common.h>
#include <mach/rockchip-regs.h>

#define RK3188_CLK_BASE		0x20000000
#define RK3188_PLL_LOCK_REG	0x200080ac

#define PLL_MODE_MASK		0x3
#define PLL_MODE_SLOW		0x0
#define PLL_MODE_NORM		0x1
#define PLL_MODE_DEEP		0x2

#define PLL_RESET_DELAY(nr)	((nr * 500) / 24 + 1)

#define PLLCON0_OD_MASK		0xf
#define PLLCON0_OD_SHIFT	0
#define PLLCON0_NR_MASK		0x3f
#define PLLCON0_NR_SHIFT	8

#define PLLCON1_NF_MASK		0x1fff
#define PLLCON1_NF_SHIFT	0

#define PLLCON2_BWADJ_MASK	0xfff
#define PLLCON2_BWADJ_SHIFT	0

#define PLLCON3_RESET		(1 << 1)
#define PLLCON3_BYPASS		(1 << 0)

struct rockchip_pll_data {
	int con_base;
	int mode_offset;
	int mode_shift;
	int lock_shift;
};

struct rockchip_pll_data rk3188_plls[] = {
	{ 0x00, 0x40, 0x00, 0x06 },
	{ 0x10, 0x40, 0x04, 0x05 },
	{ 0x20, 0x40, 0x08, 0x07 },
	{ 0x30, 0x40, 0x0c, 0x08 },
};

#define HIWORD_UPDATE(val, mask, shift) \
	((val) << (shift) | (mask) << ((shift) + 16))

int rk3188_pll_set_parameters(int pll, int nr, int nf, int no)
{
	struct rockchip_pll_data *d = &rk3188_plls[pll];
	int delay = 0;

	debug("rk3188 pll %d: set param %d %d %d\n", pll, nr, nf, no);

	/* pull pll in slow mode */
	writel(HIWORD_UPDATE(PLL_MODE_SLOW, PLL_MODE_MASK, d->mode_shift),
	       RK3188_CLK_BASE + d->mode_offset);
	/* enter reset */
	writel(HIWORD_UPDATE(PLLCON3_RESET, PLLCON3_RESET, 0),
	       RK3188_CLK_BASE + d->con_base + 12);

	/* update pll values */
	writel(HIWORD_UPDATE(nr - 1, PLLCON0_NR_MASK, PLLCON0_NR_SHIFT) |
	       HIWORD_UPDATE(no - 1, PLLCON0_OD_MASK, PLLCON0_OD_SHIFT),
	       RK3188_CLK_BASE + d->con_base + 0);
	writel(HIWORD_UPDATE(nf - 1, PLLCON1_NF_MASK, PLLCON1_NF_SHIFT),
	       RK3188_CLK_BASE + d->con_base + 4);
	writel(HIWORD_UPDATE(nf >> 1, PLLCON2_BWADJ_MASK, PLLCON2_BWADJ_SHIFT),
	       RK3188_CLK_BASE + d->con_base + 8);

	/* leave reset and wait the reset_delay */
	writel(HIWORD_UPDATE(0, PLLCON3_RESET, 0),
	       RK3188_CLK_BASE + d->con_base + 12);
	udelay(PLL_RESET_DELAY(nr));

	/* wait for the pll to lock */
	while (delay++ < 24000000) {
		if (readl(RK3188_PLL_LOCK_REG) & BIT(d->lock_shift))
			break;
	}

	/* go back to normal mode */
	writel(HIWORD_UPDATE(PLL_MODE_NORM, PLL_MODE_MASK, d->mode_shift),
	       RK3188_CLK_BASE + d->mode_offset);

	return 0;
}
EXPORT_SYMBOL(rk3188_pll_set_parameters);
