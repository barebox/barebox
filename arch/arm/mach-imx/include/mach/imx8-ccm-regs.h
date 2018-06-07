#ifndef __MACH_IMX8_CCM_REGS_H__
#define __MACH_IMX8_CCM_REGS_H__

#include "ccm.h"

#define CCM_CCGR_UART1		73

/*
 * Taken from "Table 5-1. Clock Root Table" from i.MX8M Quad
 * Applications Processor Reference Manual
 */
#define UART1_CLK_ROOT		94
#define UART1_CLK_ROOT__25M_REF_CLK CCM_TARGET_ROOTn_MUX(0b000)

#endif
