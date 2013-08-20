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
#include <mach/imx1-regs.h>
#include <mach/weim.h>
#include <mach/iomux-v1.h>
#include <mach/generic.h>
#include <reset_source.h>

#define MX1_RSR MX1_SCM_BASE_ADDR
#define RSR_EXR	(1 << 0)
#define RSR_WDR	(1 << 1)

static void imx1_detect_reset_source(void)
{
	u32 val = readl((void *)MX1_RSR) & 0x3;

	switch (val) {
	case RSR_EXR:
		set_reset_source(RESET_RST);
		return;
	case RSR_WDR:
		set_reset_source(RESET_WDG);
		return;
	case 0:
		set_reset_source(RESET_POR);
		return;
	default:
		/* else keep the default 'unknown' state */
		return;
	}
}

void imx1_setup_eimcs(size_t cs, unsigned upper, unsigned lower)
{
	writel(upper, MX1_EIM_BASE_ADDR + cs * 8);
	writel(lower, MX1_EIM_BASE_ADDR + 4 + cs * 8);
}

#include <mach/esdctl.h>

int imx1_init(void)
{
	imx1_detect_reset_source();
	add_generic_device("imx1-sdramc", 0, NULL, MX1_SDRAMC_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}

int imx1_devices_init(void)
{
	imx_iomuxv1_init((void *)MX1_GPIO1_BASE_ADDR);

	add_generic_device("imx1-ccm", 0, NULL, MX1_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpt", 0, NULL, MX1_TIM1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, MX1_GPIO1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, MX1_GPIO2_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, MX1_GPIO3_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, MX1_GPIO4_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-wdt", 0, NULL, MX1_WDT_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
