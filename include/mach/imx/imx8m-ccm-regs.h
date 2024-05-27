/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX8_CCM_REGS_H__
#define __MACH_IMX8_CCM_REGS_H__

#include <mach/imx/imx8mq-regs.h>

#define IMX8M_CCM_CCGR_DDR1	5
#define IMX8M_CCM_CCGR_I2C1	23
#define IMX8M_CCM_CCGR_I2C2	24
#define IMX8M_CCM_CCGR_I2C3	25
#define IMX8M_CCM_CCGR_I2C4	26
#define IMX8M_CCM_CCGR_SCTR	57
#define IMX8M_CCM_CCGR_UART1	73
#define IMX8M_CCM_CCGR_UART2 	74
#define IMX8M_CCM_CCGR_UART3	75
#define IMX8M_CCM_CCGR_UART4	76
#define IMX8M_CCM_CCGR_GIC	92

/*
 * Taken from "Table 5-1. Clock Root Table" from i.MX8M Quad
 * Applications Processor Reference Manual
 */
#define IMX8M_ARM_A53_CLK_ROOT		0
#define IMX8M_NOC_IO_CLK_ROOT           27
#define IMX8M_DRAM_SEL_CFG		48
#define IMX8M_DRAM_ALT_CLK_ROOT		64
#define IMX8M_DRAM_APB_CLK_ROOT		65
#define IMX8M_UART1_CLK_ROOT		94
#define IMX8M_UART2_CLK_ROOT		95
#define IMX8M_UART3_CLK_ROOT		96
#define IMX8M_UART4_CLK_ROOT		97
#define IMX8M_GIC_CLK_ROOT		100
#define IMX8M_UART1_CLK_ROOT__25M_REF_CLK IMX8M_CCM_TARGET_ROOTn_MUX(0b000)

/* 0 <= n <= 190 */
#define IMX8M_CCM_CCGRn_SET(n)	(0x4004 + 16 * (n))
#define IMX8M_CCM_CCGRn_CLR(n)	(0x4008 + 16 * (n))

/* 0 <= n <= 120 */
#define IMX8M_CCM_TARGET_ROOTn(n)	(0x8000 + 128 * (n))

#define IMX8M_CCM_TARGET_ROOTn_POST_DIV(n)	((n) & 0x0000003f)
#define IMX8M_CCM_TARGET_ROOTn_PRE_DIV(n)	(((n) << 16) & 0x00070000)
#define IMX8M_CCM_TARGET_ROOTn_MUX(x)		((x) << 24)
#define IMX8M_CCM_TARGET_ROOTn_ENABLE		BIT(28)

#define IMX8M_CCM_CCGR_SETTINGn(n, s)  ((s) << ((n) * 4))
#define IMX8M_CCM_CCGR_SETTINGn_NOT_NEEDED(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b00)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED_RUN(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b01)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED_RUN_WAIT(n)	IMX8M_CCM_CCGR_SETTINGn(n, 0b10)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b11)

void imx8m_early_setup_uart_clock(void);
void imx8mm_early_clock_init(void);
void imx8mn_early_clock_init(void);
void imx8mp_early_clock_init(void);
void imx8m_clock_set_target_val(int clock_id, u32 val);
void imx8m_ccgr_clock_enable(int index);
void imx8m_ccgr_clock_disable(int index);

#endif
