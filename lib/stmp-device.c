/*
 * Freescale i.MXS common code
 *
 * Copyright (C) 2012 Wolfram Sang <w.sang@pengutronix.de>
 *
 * Based on code from LTIB:
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <io.h>
#include <stmp-device.h>
#include <errno.h>
#include <clock.h>

#define	STMP_IP_RESET_TIMEOUT	(10 * MSECOND)

#define	STMP_BLOCK_SFTRST				(1 << 31)
#define	STMP_BLOCK_CLKGATE				(1 << 30)

int stmp_reset_block(void __iomem *reg, int just_enable)
{
	/* Clear SFTRST */
	writel(STMP_BLOCK_SFTRST, reg + STMP_OFFSET_REG_CLR);

	if (wait_on_timeout(STMP_IP_RESET_TIMEOUT, !(readl(reg) & STMP_BLOCK_SFTRST)))
		goto timeout;

	/* Clear CLKGATE */
	writel(STMP_BLOCK_CLKGATE, reg + STMP_OFFSET_REG_CLR);

	if (!just_enable) {
		/* Set SFTRST */
		writel(STMP_BLOCK_SFTRST, reg + STMP_OFFSET_REG_SET);

		/* Wait for CLKGATE being set */
		if (wait_on_timeout(STMP_IP_RESET_TIMEOUT, readl(reg) & STMP_BLOCK_CLKGATE))
			goto timeout;
	}

	/* Clear SFTRST */
	writel(STMP_BLOCK_SFTRST, reg + STMP_OFFSET_REG_CLR);

	if (wait_on_timeout(STMP_IP_RESET_TIMEOUT, !(readl(reg) & STMP_BLOCK_SFTRST)))
		goto timeout;

	/* Clear CLKGATE */
	writel(STMP_BLOCK_CLKGATE, reg + STMP_OFFSET_REG_CLR);

	if (wait_on_timeout(STMP_IP_RESET_TIMEOUT, !(readl(reg) & STMP_BLOCK_CLKGATE)))
		goto timeout;

	return 0;

timeout:
	printf("MXS: Timeout resetting block via register 0x%p\n", reg);
	return -ETIMEDOUT;
}
