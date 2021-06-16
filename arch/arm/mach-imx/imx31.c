// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <io.h>
#include <mach/imx31-regs.h>
#include <mach/weim.h>
#include <mach/generic.h>

void imx31_setup_weimcs(size_t cs, unsigned upper, unsigned lower,
		unsigned additional)
{
	writel(upper, MX31_WEIM_BASE_ADDR + (cs * 0x10) + 0x0);
	writel(lower, MX31_WEIM_BASE_ADDR + (cs * 0x10) + 0x4);
	writel(additional, MX31_WEIM_BASE_ADDR + (cs * 0x10) + 0x8);
}

int imx31_init(void)
{
	add_generic_device("imx31-esdctl", 0, NULL, MX31_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	return 0;
}

int imx31_devices_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX31_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	add_generic_device("imx31-iomux", 0, NULL, MX31_IOMUXC_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-ccm", 0, NULL, MX31_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX31_GPT1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX31_GPIO1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX31_GPIO2_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX31_GPIO3_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX31_WDOG_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-usb-misc", 0, NULL, MX31_USB_OTG_BASE_ADDR + 0x600, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
