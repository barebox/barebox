#ifndef __ASM_MACH_ATH79_PBL_MACROS_H
#define __ASM_MACH_ATH79_PBL_MACROS_H

#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <mach/ar71xx_regs.h>

#define PLL_BASE		(KSEG1 | AR71XX_PLL_BASE)
#define PLL_CPU_CONFIG_REG	(PLL_BASE | AR933X_PLL_CPU_CONFIG_REG)
#define PLL_CPU_CONFIG2_REG	(PLL_BASE | AR933X_PLL_CPU_CONFIG2_REG)
#define PLL_CLOCK_CTRL_REG	(PLL_BASE | AR933X_PLL_CLOCK_CTRL_REG)

#define DEF_25MHZ_PLL_CLOCK_CTRL \
				((2 - 1) << AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT)
#define DEF_25MHZ_SETTLE_TIME	(34000 / 40)
#define DEF_25MHZ_PLL_CONFIG	( 1 << AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT \
				| 1 << AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT \
				| 32 << AR933X_PLL_CPU_CONFIG_NINT_SHIFT)

.macro	pbl_ar9331_pll
	.set	push
	.set	noreorder

	/* Most devices have 25 MHz Ref clock. */
	pbl_reg_writel (DEF_25MHZ_PLL_CLOCK_CTRL | AR933X_PLL_CLOCK_CTRL_BYPASS), \
		PLL_CLOCK_CTRL_REG
	pbl_reg_writel DEF_25MHZ_SETTLE_TIME, PLL_CPU_CONFIG2_REG
	pbl_reg_writel (DEF_25MHZ_PLL_CONFIG | AR933X_PLL_CPU_CONFIG_PLLPWD), \
		PLL_CPU_CONFIG_REG

	/* power on CPU PLL */
	pbl_reg_clr	AR933X_PLL_CPU_CONFIG_PLLPWD, PLL_CPU_CONFIG_REG
	/* disable PLL bypass */
	pbl_reg_clr	AR933X_PLL_CLOCK_CTRL_BYPASS, PLL_CLOCK_CTRL_REG

	pbl_sleep	t2, 40

	.set	pop
.endm

#define DDR_BASE		(KSEG1 | AR71XX_DDR_CTRL_BASE)
#define DDR_CONFIG		(DDR_BASE | AR933X_DDR_CONFIG)
#define DDR_CONFIG2		(DDR_BASE | AR933X_DDR_CONFIG2)
#define DDR_MODE		(DDR_BASE | AR933X_DDR_MODE)
#define DDR_EXT_MODE		(DDR_BASE | AR933X_DDR_EXT_MODE)

#define DDR_CTRL		(DDR_BASE | AR933X_DDR_CTRL)
/* Forces an EMR3S (Extended Mode Register 3 Set) update cycle */
#define DDR_CTRL_EMR3		BIT(5)
/* Forces an EMR2S (Extended Mode Register 2 Set) update cycle */
#define DDR_CTRL_EMR2		BIT(4)
#define DDR_CTRL_PREA		BIT(3) /* Forces a PRECHARGE ALL cycle */
#define DDR_CTRL_REF		BIT(2) /* Forces an AUTO REFRESH cycle */
/* Forces an EMRS (Extended Mode Register 2 Set) update cycle */
#define DDR_CTRL_EMRS		BIT(1)
/* Forces a MRS (Mode Register Set) update cycle */
#define DDR_CTRL_MRS		BIT(0)

#define DDR_REFRESH		(DDR_BASE | AR933X_DDR_REFRESH)
#define DDR_RD_DATA		(DDR_BASE | AR933X_DDR_RD_DATA)
#define DDR_TAP_CTRL0		(DDR_BASE | AR933X_DDR_TAP_CTRL0)
#define DDR_TAP_CTRL1		(DDR_BASE | AR933X_DDR_TAP_CTRL1)

#define DDR_DDR2_CONFIG		(DDR_BASE | AR933X_DDR_DDR_DDR2_CONFIG)
#define DDR_EMR2		(DDR_BASE | AR933X_DDR_DDR_EMR2)
#define DDR_EMR3		(DDR_BASE | AR933X_DDR_DDR_EMR3)

.macro	pbl_ar9331_ddr2_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x7fbc8cd0, DDR_CONFIG
	pbl_reg_writel	0x9dd0e6a8, DDR_CONFIG2

	/* Enable DDR2 */
	pbl_reg_writel	0x00000a59, DDR_DDR2_CONFIG
	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* Disable High Temperature Self-Refresh Rate */
	pbl_reg_writel	0x00000000, DDR_EMR2
	pbl_reg_writel	DDR_CTRL_EMR2, DDR_CTRL

	pbl_reg_writel	0x00000000, DDR_EMR3
	pbl_reg_writel	DDR_CTRL_EMR3, DDR_CTRL

	/* Enable DLL */
	pbl_reg_writel	0x00000000, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/* Reset DLL */
	pbl_reg_writel	0x00000100, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL
	pbl_reg_writel	DDR_CTRL_REF, DDR_CTRL
	pbl_reg_writel	DDR_CTRL_REF, DDR_CTRL

	/* Write recovery (WR) 6 clock, CAS Latency 3, Burst Length 8 */
	pbl_reg_writel	0x00000a33, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[9:7] = 0x7: (OCD Calibration defaults)
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x00000382, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[9:7] = 0x0: (OCD exit)
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x00000402, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/* Refresh control. Bit 14 is enable. Bits <13:0> Refresh time */
	pbl_reg_writel	0x00004186, DDR_REFRESH
	/* DQS 0 Tap Control (needs tuning) */
	pbl_reg_writel	0x00000008, DDR_TAP_CTRL0
	/* DQS 1 Tap Control (needs tuning) */
	pbl_reg_writel	0x00000009, DDR_TAP_CTRL1
	/* For 16-bit DDR */
	pbl_reg_writel	0x000000ff, DDR_RD_DATA

	.set	pop
.endm

#define GPIO_FUNC	((KSEG1 | AR71XX_GPIO_BASE) | AR71XX_GPIO_REG_FUNC)

.macro	pbl_ar9331_uart_enable
	pbl_reg_set AR933X_GPIO_FUNC_UART_EN \
			| AR933X_GPIO_FUNC_RSRV15, GPIO_FUNC
.endm

#endif /* __ASM_MACH_ATH79_PBL_MACROS_H */
