/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx8-ccm-regs.h>
#include <mach/iomux-mx8.h>
#include <mach/imx8-ddrc.h>
#include <mach/xload.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/mmu.h>

#include "ddr.h"

extern char __dtb_imx8mq_evk_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *iomux = IOMEM(MX8MQ_IOMUXC_BASE_ADDR);
	void __iomem *ccm   = IOMEM(MX8MQ_CCM_BASE_ADDR);

	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_CLR(CCM_CCGR_UART1));
	writel(CCM_TARGET_ROOTn_ENABLE | UART1_CLK_ROOT__25M_REF_CLK,
	       ccm + CCM_TARGET_ROOTn(UART1_CLK_ROOT));
	writel(CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + CCM_CCGRn_SET(CCM_CCGR_UART1));

	imx_setup_pad(iomux, IMX8MQ_PAD_UART1_TXD__UART1_TX | UART_PAD_CTRL);

	imx8_uart_setup_ll();

	putc_ll('>');
}

static void nxp_imx8mq_evk_sram_setup(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = BOOTSOURCE_INSTANCE_UNKNOWN;
	int ret = -ENOTSUPP;

	ddr_init();

	imx8_get_boot_source(&src, &instance);

	if (src == BOOTSOURCE_MMC)
		ret = imx8_esdhc_start_image(instance);

	BUG_ON(ret);
}

ENTRY_FUNCTION(start_nxp_imx8mq_evk, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	if (get_pc() < MX8MQ_DDR_CSD1_BASE_ADDR)
		nxp_imx8mq_evk_sram_setup();

	barebox_arm_entry(MX8MQ_DDR_CSD1_BASE_ADDR,
			  SZ_2G + SZ_1G, __dtb_imx8mq_evk_start);
}

