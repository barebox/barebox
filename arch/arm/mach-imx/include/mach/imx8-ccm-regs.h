#ifndef __MACH_IMX8_CCM_REGS_H__
#define __MACH_IMX8_CCM_REGS_H__

#define IMX8M_CCM_CCGR_UART1		73

/*
 * Taken from "Table 5-1. Clock Root Table" from i.MX8M Quad
 * Applications Processor Reference Manual
 */
#define IMX8M_UART1_CLK_ROOT		94
#define IMX8M_UART1_CLK_ROOT__25M_REF_CLK IMX8M_CCM_TARGET_ROOTn_MUX(0b000)

/* 0 <= n <= 190 */
#define IMX8M_CCM_CCGRn_SET(n)	(0x4004 + 16 * (n))
#define IMX8M_CCM_CCGRn_CLR(n)	(0x4008 + 16 * (n))

/* 0 <= n <= 120 */
#define IMX8M_CCM_TARGET_ROOTn(n)	(0x8000 + 128 * (n))

#define IMX8M_CCM_TARGET_ROOTn_MUX(x)		((x) << 24)
#define IMX8M_CCM_TARGET_ROOTn_ENABLE		BIT(28)


#define IMX8M_CCM_CCGR_SETTINGn(n, s)  ((s) << ((n) * 4))
#define IMX8M_CCM_CCGR_SETTINGn_NOT_NEEDED(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b00)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED_RUN(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b01)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED_RUN_WAIT(n)	IMX8M_CCM_CCGR_SETTINGn(n, 0b10)
#define IMX8M_CCM_CCGR_SETTINGn_NEEDED(n)		IMX8M_CCM_CCGR_SETTINGn(n, 0b11)

#endif
