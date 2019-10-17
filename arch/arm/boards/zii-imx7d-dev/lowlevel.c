// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2018 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <debug_ll.h>
#include <io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx7-ccm-regs.h>
#include <mach/iomux-mx7.h>
#include <mach/debug_ll.h>
#include <asm/cache.h>
#include <mach/esdctl.h>

extern char __dtb_z_imx7d_zii_rpu2_start[];
extern char __dtb_z_imx7d_zii_rmu2_start[];

static inline void setup_uart(void)
{
	void __iomem *iomux = IOMEM(MX7_IOMUXC_BASE_ADDR);
	void __iomem *ccm   = IOMEM(MX7_CCM_BASE_ADDR);

	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_CLR(CCM_CCGR_UART2));
	writel(CCM_TARGET_ROOTn_ENABLE | UART2_CLK_ROOT__OSC_24M,
	       ccm + CCM_TARGET_ROOTn(UART2_CLK_ROOT));
	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_SET(CCM_CCGR_UART2));

	mx7_setup_pad(iomux, MX7D_PAD_UART2_TX_DATA__UART2_DCE_TX);

	imx7_uart_setup_ll();

	putc_ll('>');
}

enum zii_platform_imx7d_type {
	ZII_PLATFORM_IMX7D_RPU2 = 0b0000,
	ZII_PLATFORM_IMX7D_RMU2 = 0b1000,
};

static unsigned int get_system_type(void)
{
#define GPIO_DR		0x000
#define GPIO_GDIR	0x004
#define SYSTEM_TYPE	GENMASK(15, 12)

	u32 gdir, dr;
	void __iomem *gpio2 = IOMEM(MX7_GPIO2_BASE_ADDR);
	void __iomem *iomuxbase = IOMEM(MX7_IOMUXC_BASE_ADDR);

	/*
	 * System type is encoded as a 4-bit number specified by the
	 * following pins (pulled up or down with resistors on the
	 * board).
	 */
	imx_setup_pad(iomuxbase, MX7D_PAD_EPDC_DATA12__GPIO2_IO12);
	imx_setup_pad(iomuxbase, MX7D_PAD_EPDC_DATA13__GPIO2_IO13);
	imx_setup_pad(iomuxbase, MX7D_PAD_EPDC_DATA14__GPIO2_IO14);
	imx_setup_pad(iomuxbase, MX7D_PAD_EPDC_DATA15__GPIO2_IO15);

	gdir = readl(gpio2 + GPIO_GDIR);
	gdir &= ~SYSTEM_TYPE;
	writel(gdir, gpio2 + GPIO_GDIR);

	dr = readl(gpio2 + GPIO_DR);

	return FIELD_GET(SYSTEM_TYPE, dr);
}

ENTRY_FUNCTION(start_zii_imx7d_dev, r0, r1, r2)
{
	void *fdt;
	const unsigned int system_type = get_system_type();

	imx7_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	switch (system_type) {
	default:
		if (IS_ENABLED(CONFIG_DEBUG_LL)) {
			relocate_to_current_adr();
			setup_c();
			puts_ll("\n*********************************\n");
			puts_ll("* Unknown system type: ");
			puthex_ll(system_type);
			puts_ll("\n* Assuming RPU2\n");
			puts_ll("*********************************\n");
		}
		/* FALLTHROUGH */
	case ZII_PLATFORM_IMX7D_RPU2:
		fdt = __dtb_z_imx7d_zii_rpu2_start;
		break;
	case ZII_PLATFORM_IMX7D_RMU2:
		fdt = __dtb_z_imx7d_zii_rmu2_start;
		break;
	}

	imx7d_barebox_entry(fdt + get_runtime_offset());
}
