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
#include <mach/imx21-regs.h>
#include <mach/weim.h>
#include <mach/iomux-v1.h>
#include <mach/generic.h>

void imx21_setup_eimcs(size_t cs, unsigned upper, unsigned lower)
{
	writel(upper, MX21_EIM_BASE_ADDR + cs * 8);
	writel(lower, MX21_EIM_BASE_ADDR + 4 + cs * 8);
}

int imx21_init(void)
{
	return 0;
}

int imx21_devices_init(void)
{
	imx_iomuxv1_init((void *)MX21_GPIO1_BASE_ADDR);

	add_generic_device("imx21-ccm", 0, NULL, MX21_CCM_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpt", 0, NULL, MX21_GPT1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, MX21_GPIO1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, MX21_GPIO2_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, MX21_GPIO3_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, MX21_GPIO4_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 4, NULL, MX21_GPIO5_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 5, NULL, MX21_GPIO6_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX21_WDOG_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
