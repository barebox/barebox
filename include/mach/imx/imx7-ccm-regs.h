/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX7_CCM_REGS_H__
#define __MACH_IMX7_CCM_REGS_H__

#include <io.h>
#include <linux/build_bug.h>
#include <mach/imx/imx7-regs.h>

#define IMX7_CLOCK_ROOT_INDEX(x)	(((x) - 0x8000) / 128)

/*
 * Taken from "Table 5-11. Clock Root Table" from i.MX7 Dual Processor
 * Reference Manual
 */

/* 1 <= n <= 6 */
#define IMX7_CCM_CCGR_UART(n)		(148 + (n) - 1)
#define IMX7_UART_CLK_ROOT(n)		IMX7_CLOCK_ROOT_INDEX(0xaf80 + (n - 1) * 0x80)
#define IMX7_UART_CLK_ROOT__OSC_24M	IMX7_CCM_TARGET_ROOTn_MUX(0b000)

/* 0 <= n <= 190 */
#define IMX7_CCM_CCGRn_SET(n)	(0x4004 + 16 * (n))
#define IMX7_CCM_CCGRn_CLR(n)	(0x4008 + 16 * (n))

/* 0 <= n <= 120 */
#define IMX7_CCM_TARGET_ROOTn(n)	(0x8000 + 128 * (n))

#define IMX7_CCM_TARGET_ROOTn_MUX(x)		((x) << 24)
#define IMX7_CCM_TARGET_ROOTn_ENABLE		BIT(28)


#define IMX7_CCM_CCGR_SETTINGn(n, s)  ((s) << ((n) * 4))
#define IMX7_CCM_CCGR_SETTINGn_NOT_NEEDED(n)		IMX7_CCM_CCGR_SETTINGn(n, 0b00)
#define IMX7_CCM_CCGR_SETTINGn_NEEDED_RUN(n)		IMX7_CCM_CCGR_SETTINGn(n, 0b01)
#define IMX7_CCM_CCGR_SETTINGn_NEEDED_RUN_WAIT(n)	IMX7_CCM_CCGR_SETTINGn(n, 0b10)
#define IMX7_CCM_CCGR_SETTINGn_NEEDED(n)		IMX7_CCM_CCGR_SETTINGn(n, 0b11)

/* UART counting starts for 1, like in the datasheet/dt-bindings */

static inline void __imx7_early_setup_uart_clock(int uart)
{
	void __iomem *ccm   = IOMEM(MX7_CCM_BASE_ADDR);

	writel(IMX7_CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + IMX7_CCM_CCGRn_CLR(IMX7_CCM_CCGR_UART(uart)));
	writel(IMX7_CCM_TARGET_ROOTn_ENABLE | IMX7_UART_CLK_ROOT__OSC_24M,
	       ccm + IMX7_CCM_TARGET_ROOTn(IMX7_UART_CLK_ROOT(uart)));
	writel(IMX7_CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + IMX7_CCM_CCGRn_SET(IMX7_CCM_CCGR_UART(uart)));
}

#define imx7_early_setup_uart_clock(uart) do {	\
	static_assert(1 <= (uart) && (uart) <= 6, "ID out of UART1-6 range"); \
	__imx7_early_setup_uart_clock(uart); \
} while (0)

#endif
