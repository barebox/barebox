#ifndef __MACH_IMX7_CCM_REGS_H__
#define __MACH_IMX7_CCM_REGS_H__

#include "ccm.h"

#define CCM_CCGR_UART1		148
#define CCM_CCGR_UART2		149

#define CLOCK_ROOT_INDEX(x)	(((x) - 0x8000) / 128)

/*
 * Taken from "Table 5-11. Clock Root Table" from i.MX7 Dual Processor
 * Reference Manual
 */
#define UART1_CLK_ROOT		CLOCK_ROOT_INDEX(0xaf80)
#define UART1_CLK_ROOT__OSC_24M CCM_TARGET_ROOTn_MUX(0b000)

#define UART2_CLK_ROOT		CLOCK_ROOT_INDEX(0xb000)
#define UART2_CLK_ROOT__OSC_24M CCM_TARGET_ROOTn_MUX(0b000)

#endif
