/*
 * Copyright (C) 2012 Alexey Galakhov
 *
 * Based on code from u-boot found somewhere on the web
 * that seems to originate from Samsung
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include <common.h>
#include <io.h>
#include <init.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-clocks.h>
#include <mach/s3c-generic.h>

#ifdef CONFIG_S3C_PLL_INIT
void  __bare_init s5p_init_pll(void)
{
    uint32_t reg;
    int i;

    /* Set Mux to FIN */
    writel(0, S5P_CLK_SRC0);

    writel(BOARD_APLL_LOCKTIME, S5P_xPLL_LOCK + S5P_APLL);

    /* Disable PLL */
    writel(0, S5P_xPLL_CON + S5P_APLL);
    writel(0, S5P_xPLL_CON + S5P_MPLL);

    /* Set up dividers */
    reg = readl(S5P_CLK_DIV0);
    reg &= ~(BOARD_CLK_DIV0_MASK);
    reg |= (BOARD_CLK_DIV0_VAL);
    writel(reg, S5P_CLK_DIV0);

    /* Set up PLLs */
    writel(BOARD_APLL_VAL, S5P_xPLL_CON + S5P_APLL);
    writel(BOARD_MPLL_VAL, S5P_xPLL_CON + S5P_MPLL);
    writel(BOARD_EPLL_VAL, S5P_xPLL_CON + S5P_EPLL);
    writel(BOARD_VPLL_VAL, S5P_xPLL_CON + S5P_VPLL);

    /* Wait for sync */
    for (i = 0; i < 0x10000; ++i)
        barrier();

    reg = readl(S5P_CLK_SRC0);
    reg |= 0x1111; /* switch MUX to PLL outputs */
    writel(reg, S5P_CLK_SRC0);
}
#endif /* CONFIG_S3C_PLL_INIT */
