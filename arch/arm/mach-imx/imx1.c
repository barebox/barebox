/*
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
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <mach/weim.h>

void imx1_setup_eimcs(size_t cs, unsigned upper, unsigned lower)
{
	writel(upper, MX1_EIM_BASE_ADDR + cs * 8);
	writel(lower, MX1_EIM_BASE_ADDR + 4 + cs * 8);
}

static int imx1_init(void)
{
	add_generic_device("imx1-gpt", 0, NULL, MX1_TIM1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, MX1_GPIO1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, MX1_GPIO2_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, MX1_GPIO3_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, MX1_GPIO4_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx1_init);
