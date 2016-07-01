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
#include <linux/sizes.h>
#include <init.h>
#include <io.h>
#include <mach/weim.h>
#include <mach/imx35-regs.h>
#include <mach/iim.h>
#include <mach/revision.h>
#include <mach/generic.h>

void imx35_setup_weimcs(size_t cs, unsigned upper, unsigned lower,
		unsigned additional)
{
	writel(upper, MX35_WEIM_BASE_ADDR + (cs * 0x10) + 0x0);
	writel(lower, MX35_WEIM_BASE_ADDR + (cs * 0x10) + 0x4);
	writel(additional, MX35_WEIM_BASE_ADDR + (cs * 0x10) + 0x8);
}

static void imx35_silicon_revision(void)
{
	uint32_t reg;
	reg = readl(MX35_IIM_BASE_ADDR + IIM_SREV);
	/* 0×00 = TO 1.0, First silicon */
	reg += IMX_CHIP_REV_1_0;

	imx_set_silicon_revision("i.MX35", reg & 0xFF);
}

/*
 * There are some i.MX35 CPUs in the wild, comming with bogus L2 cache settings.
 * These misconfigured CPUs will run amok immediately when the L2 cache gets
 * enabled. Workaraound is to setup the correct register setting prior enabling
 * the L2 cache. This should not hurt already working CPUs, as they are using the
 * same value
 */

#define L2_MEM_VAL 0x10

int imx35_init(void)
{
	writel(0x515, MX35_CLKCTL_BASE_ADDR + L2_MEM_VAL);

	imx35_silicon_revision();

	imx35_boot_save_loc();
	add_generic_device("imx35-esdctl", 0, NULL, MX35_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);

	return 0;
}

int imx35_devices_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX35_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	add_generic_device("imx-iomuxv3", 0, NULL, MX35_IOMUXC_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx35-ccm", 0, NULL, MX35_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX35_GPT1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX35_GPIO1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX35_GPIO2_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX35_GPIO3_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX35_WDOG_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx35-usb-misc", 0, NULL, MX35_USB_OTG_BASE_ADDR + 0x600, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
