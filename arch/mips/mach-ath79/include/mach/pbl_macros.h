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

.macro	pbl_ar9331_ddr1_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x7fbc8cd0, DDR_CONFIG
	pbl_reg_writel	0x9dd0e6a8, DDR_CONFIG2

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* 0x133: on reset Mode Register value */
	pbl_reg_writel	0x133, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x2, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* DLL out of reset, CAS Latency 3 */
	pbl_reg_writel	0x33, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/* Refresh control. Bit 14 is enable. Bits<13:0> Refresh time */
	pbl_reg_writel	0x4186, DDR_REFRESH
	/* This register is used along with DQ Lane 0; DQ[7:0], DQS_0 */
	pbl_reg_writel	0x8, DDR_TAP_CTRL0
	/* This register is used along with DQ Lane 1; DQ[15:8], DQS_1 */
	pbl_reg_writel	0x9, DDR_TAP_CTRL1

	/*
	 * DDR read and capture bit mask.
	 * Each bit represents a cycle of valid data.
	 * 0xff: use 16-bit DDR
	 */
	pbl_reg_writel	0xff, DDR_RD_DATA

	.set	pop
.endm

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

#define RESET_REG_BOOTSTRAP	((KSEG1 | AR71XX_RESET_BASE) \
					| AR933X_RESET_REG_BOOTSTRAP)

.macro	pbl_ar9331_ram_generic_config
	.set	push
	.set	noreorder

	li	t5,	RESET_REG_BOOTSTRAP
	/* Documentation and source code of existing boot loaders disagree at
	 * this place. Doc says: MEM_TYPE[13:12]:
	 * - 00 = SDRAM
	 * - 01 = DDR1
	 * - 10 = DDR2
	 * The source code of most loaders do not care about BIT(12). So we do
	 * the same.
	 */
	li	t6,	AR933X_BOOTSTRAP_MEM_TYPE
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	pbl_ar9331_ram_generic_ddr1
	nop

pbl_ar9331_ram_generic_ddr2:
	pbl_ar9331_ddr2_config
	b	pbl_ar9331_ram_generic_config
	nop

pbl_ar9331_ram_generic_ddr1:
	pbl_ar9331_ddr1_config

pbl_ar9331_ram_generic_config:
	.set	pop
.endm

#define GPIO_FUNC	((KSEG1 | AR71XX_GPIO_BASE) | AR71XX_GPIO_REG_FUNC)

.macro	pbl_ar9331_uart_enable
	pbl_reg_set AR933X_GPIO_FUNC_UART_EN \
			| AR933X_GPIO_FUNC_RSRV15, GPIO_FUNC
.endm

.macro	pbl_ar9331_mdio_gpio_enable
	/* Bit 18 enables MDC and MDIO function on GPIO26 and GPIO28 */
	pbl_reg_set (1 << 18), RESET_REG_BOOTSTRAP
.endm

.macro	hornet_mips24k_cp0_setup
	.set push
	.set noreorder

	/*
	 * Clearing CP0 registers - This is generally required for the MIPS-24k
	 * core used by Atheros.
	 */
	mtc0	zero, CP0_INDEX
	mtc0	zero, CP0_ENTRYLO0
	mtc0	zero, CP0_ENTRYLO1
	mtc0	zero, CP0_CONTEXT
	mtc0	zero, CP0_PAGEMASK
	mtc0	zero, CP0_WIRED
	mtc0	zero, CP0_INFO
	mtc0	zero, CP0_COUNT
	mtc0	zero, CP0_ENTRYHI
	mtc0	zero, CP0_COMPARE

	li	t0, ST0_CU0 | ST0_ERL
	mtc0	t0, CP0_STATUS

	mtc0	zero, CP0_CAUSE
	mtc0	zero, CP0_EPC

	li	t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG

	mtc0	zero, CP0_LLADDR
	mtc0	zero, CP0_WATCHLO
	mtc0	zero, CP0_WATCHHI
	mtc0	zero, CP0_XCONTEXT
	mtc0	zero, CP0_FRAMEMASK
	mtc0	zero, CP0_DIAGNOSTIC
	mtc0	zero, CP0_DEBUG
	mtc0	zero, CP0_DEPC
	mtc0	zero, CP0_PERFORMANCE
	mtc0	zero, CP0_ECC
	mtc0	zero, CP0_CACHEERR
	mtc0	zero, CP0_TAGLO

	.set	pop
.endm

.macro	hornet_1_1_war
	.set push
	.set noreorder

/*
 * WAR: Hornet 1.1 currently need a reset once we boot to let the resetb has
 *      enough time to stable, so that trigger reset at 1st boot, system team
 *      is investigaing the issue, will remove in short
 */

	li  t7, 0xbd000000
	lw  t8, 0(t7)
	li  t9, 0x12345678

	/* if value of 0xbd000000 != 0x12345678, go to do_reset */
	bne t8, t9, do_reset
	 nop

	li  t9, 0xffffffff
	sw  t9, 0(t7)
	b   normal_path
	 nop

do_reset:
	/* put 0x12345678 into 0xbd000000 */
	sw  t9, 0(t7)

	/* reset register 0x1806001c */
	li  t7, 0xb806001c
	lw  t8, 0(t7)
	/* bit24, fullchip reset */
	li  t9, 0x1000000
	or  t8, t8, t9
	sw  t8, 0(t7)

do_reset_loop:
	b   do_reset_loop
	 nop

normal_path:
	.set	pop
.endm

.macro	pbl_ar9331_wmac_enable
	.set push
	.set noreorder

	/* These three WLAN_RESET will avoid original issue */
	li      t3, 0x03
1:
	li      t0, CKSEG1ADDR(AR71XX_RESET_BASE)
	lw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	ori     t1, t1, 0x0800
	sw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	nop
	lw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	li      t2, 0xfffff7ff
	and     t1, t1, t2
	sw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	nop
	addi    t3, t3, -1
	bnez    t3, 1b
	nop

	li      t2, 0x20
2:
	beqz    t2, 1b
	nop
	addi    t2, t2, -1
	lw      t5, AR933X_RESET_REG_BOOTSTRAP(t0)
	andi    t1, t5, 0x10
	bnez    t1, 2b
	nop

	li      t1, 0x02110E
	sw      t1, AR933X_RESET_REG_BOOTSTRAP(t0)
	nop

	/* RTC Force Wake */
	li      t0, CKSEG1ADDR(AR71XX_RTC_BASE)
	li      t1, 0x03
	sw      t1, AR933X_RTC_REG_FORCE_WAKE(t0)
	nop
	nop

	/* RTC Reset */
	li      t1, 0x00
	sw      t1, AR933X_RTC_REG_RESET(t0)
	nop
	nop

	li      t1, 0x01
	sw      t1, AR933X_RTC_REG_RESET(t0)
	nop
	nop

	/* Wait for RTC in on state */
1:
	lw      t1, AR933X_RTC_REG_STATUS(t0)
	andi    t1, t1, 0x02
	beqz    t1, 1b
	nop

	.set	pop
.endm

	.macro	ar9331_pbl_generic_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	hornet_mips24k_cp0_setup

	pbl_ar9331_wmac_enable

	hornet_1_1_war

	pbl_ar9331_pll
	pbl_ar9331_ram_generic_config

skip_pll_ram_config:
	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

	pbl_ar9331_uart_enable
	debug_ll_ar9331_init
	mips_nmon

	pbl_ar9331_mdio_gpio_enable

	copy_to_link_location	pbl_start

	.set	pop
	.endm

#endif /* __ASM_MACH_ATH79_PBL_MACROS_H */
