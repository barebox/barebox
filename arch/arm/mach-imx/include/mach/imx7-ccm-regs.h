#ifndef __MACH_IMX7_CCM_REGS_H__
#define __MACH_IMX7_CCM_REGS_H__

/* 0 <= n <= 190 */
#define CCM_CCGRn_SET(n)	(0x4004 + 16 * (n))
#define CCM_CCGRn_CLR(n)	(0x4008 + 16 * (n))

#define CCM_CCGR_UART1		148

#define CCM_CCGR_SETTINGn(n, s)  ((s) << ((n) * 4))
#define CCM_CCGR_SETTINGn_NOT_NEEDED(n)		CCM_CCGR_SETTINGn(n, 0b00)
#define CCM_CCGR_SETTINGn_NEEDED_RUN(n)		CCM_CCGR_SETTINGn(n, 0b01)
#define CCM_CCGR_SETTINGn_NEEDED_RUN_WAIT(n)	CCM_CCGR_SETTINGn(n, 0b10)
#define CCM_CCGR_SETTINGn_NEEDED(n)		CCM_CCGR_SETTINGn(n, 0b11)

/* 0 <= n <= 120 */
#define CCM_TARGET_ROOTn(n)	(0x8000 + 128 * (n))

#define CCM_TARGET_ROOTn_MUX(x)		((x) << 24)
#define CCM_TARGET_ROOTn_ENABLE		BIT(28)

#define CLOCK_ROOT_INDEX(x)	(((x) - 0x8000) / 128)

/*
 * Taken from "Table 5-11. Clock Root Table" from i.MX7 Dual Processor
 * Reference Manual
 */
#define UART1_CLK_ROOT		CLOCK_ROOT_INDEX(0xaf80)
#define UART1_CLK_ROOT__OSC_24M CCM_TARGET_ROOTn_MUX(0b000)


#endif
