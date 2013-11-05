/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
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
 *
 * Minimal GPIO support.
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/immap_85xx.h>

#ifdef CONFIG_MPC8544
/* Enable all GPIO output pins */
void fsl_enable_gpiout(void)
{
	void __iomem *gpiocr = IOMEM(MPC85xx_GUTS_ADDR + MPC85xx_GPIOCR_OFFSET);

	out_be32(gpiocr, in_be32(gpiocr) | MPC85xx_GPIOCR_GPOUT);
}

void gpio_set_value(unsigned gpio, int val)
{
	void __iomem *gpout = IOMEM(MPC85xx_GUTS_ADDR + MPC85xx_GPOUTDR_OFFSET);
	int gpoutdr;

	if (gpio >= 8)
		return;

	gpoutdr = in_be32(gpout);
	if (val)
		gpoutdr |= MPC85xx_GPIOBIT(gpio);
	else
		gpoutdr &= ~MPC85xx_GPIOBIT(gpio);
	out_be32(gpout, gpoutdr);
}
#endif
