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
#include <mach/imx27-regs.h>
#include <mach/weim.h>
#include <mach/iomux-v1.h>
#include <linux/sizes.h>
#include <mach/revision.h>
#include <mach/generic.h>
#include <init.h>
#include <io.h>
#include <mach/generic.h>

static int imx27_silicon_revision(void)
{
	uint32_t val;
	int rev;

	val = readl(MX27_SYSCTRL_BASE_ADDR);

	switch (val >> 28) {
	case 0:
		rev = IMX_CHIP_REV_1_0;
		break;
	case 1:
		rev = IMX_CHIP_REV_2_0;
		break;
	case 2:
		rev = IMX_CHIP_REV_2_1;
		break;
	default:
		rev = IMX_CHIP_REV_UNKNOWN;
		break;
	}

	imx_set_silicon_revision("i.MX27", rev);

	return 0;
}

void imx27_setup_weimcs(size_t cs, unsigned upper, unsigned lower,
		unsigned additional)
{
	writel(upper, MX27_WEIM_BASE_ADDR + (cs * 0x10) + 0x0);
	writel(lower, MX27_WEIM_BASE_ADDR + (cs * 0x10) + 0x4);
	writel(additional, MX27_WEIM_BASE_ADDR + (cs * 0x10) + 0x8);
}

/*
 * Initialize MAX on i.MX27. necessary to give the DMA engine
 * higher priority to the memory than the CPU. Needed for proper
 * audio support
 */
#define MAX_SLAVE_MPR_OFFSET	0x0	/* Master Priority register */
#define MAX_SLAVE_AMPR_OFFSET	0x4	/* Alternate Master Priority register */
#define MAX_SLAVE_PORT0_OFFSET	0x0
#define MAX_SLAVE_PORT1_OFFSET	0x100
#define MAX_SLAVE_PORT2_OFFSET	0x200
#define MAX_MASTER_PRIO(master, prio)	(((prio) << (master) * 4))

#define MASTER_IAHB	0
#define MASTER_DAHB	1
#define MASTER_EMMA	2
#define MASTER_DMA	3
#define MASTER_SLDC	4
#define MASTER_CODEC	5

static void imx27_init_max(void)
{
	void __iomem *max_base = (void *)MX27_MAX_BASE_ADDR;
	u32 val;

	/* 0 is the highest priority */
	val = MAX_MASTER_PRIO(MASTER_IAHB, 5) |
		MAX_MASTER_PRIO(MASTER_DAHB, 4) |
		MAX_MASTER_PRIO(MASTER_EMMA, 1) |
		MAX_MASTER_PRIO(MASTER_DMA, 2) |
		MAX_MASTER_PRIO(MASTER_SLDC, 0) |
		MAX_MASTER_PRIO(MASTER_CODEC, 3);

	writel(val, max_base + MAX_SLAVE_PORT0_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT1_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT2_OFFSET + MAX_SLAVE_MPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT0_OFFSET + MAX_SLAVE_AMPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT1_OFFSET + MAX_SLAVE_AMPR_OFFSET);
	writel(val, max_base + MAX_SLAVE_PORT2_OFFSET + MAX_SLAVE_AMPR_OFFSET);
}

int imx27_init(void)
{
	imx27_silicon_revision();
	imx27_boot_save_loc();
	add_generic_device("imx27-esdctl", DEVICE_ID_SINGLE, NULL,
			   MX27_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);

	return 0;
}

int imx27_devices_init(void)
{
	imx_iomuxv1_init((void *)MX27_GPIO1_BASE_ADDR);

	add_generic_device("imx_iim", DEVICE_ID_SINGLE, NULL,
			   MX27_IIM_BASE_ADDR, SZ_4K, IORESOURCE_MEM, NULL);

	imx27_init_max();

	add_generic_device("imx27-ccm", DEVICE_ID_SINGLE, NULL,
			   MX27_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpt", 0, NULL, MX27_GPT1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, MX27_GPIO1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, MX27_GPIO2_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, MX27_GPIO3_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, MX27_GPIO4_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 4, NULL, MX27_GPIO5_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 5, NULL, MX27_GPIO6_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX27_WDOG_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx27-usb-misc", 0, NULL, MX27_USB_OTG_BASE_ADDR + 0x600, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
