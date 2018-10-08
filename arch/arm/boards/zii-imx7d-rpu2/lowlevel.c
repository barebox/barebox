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

extern char __dtb_imx7d_zii_rpu2_start[];

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

ENTRY_FUNCTION(start_zii_imx7d_rpu2, r0, r1, r2)
{
	imx7_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	imx7d_barebox_entry(__dtb_imx7d_zii_rpu2_start + get_runtime_offset());
}
