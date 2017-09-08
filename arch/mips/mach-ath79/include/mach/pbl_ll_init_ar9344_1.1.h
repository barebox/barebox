#ifndef __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H
#define __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H

#include <asm/addrspace.h>
#include <asm/regdef.h>

#define AR7240_APB_BASE			(KSEG1 | 0x18000000)  /* 384M */
#define AR7240_DDR_CTL_BASE		AR7240_APB_BASE+0x00000000
#define AR7240_PLL_BASE			AR7240_APB_BASE+0x00050000

#define	ATH_DDR_COUNT_LOC		(KSEG1 | 0x1d000000)
#define	ATH_CPU_COUNT_LOC		(KSEG1 | 0x1d000004)

/*
 * DDR block
 */
#define AR7240_DDR_CONFIG		AR7240_DDR_CTL_BASE+0
#define AR7240_DDR_CONFIG2		AR7240_DDR_CTL_BASE+4
#define AR7240_DDR_MODE			AR7240_DDR_CTL_BASE+0x08
#define AR7240_DDR_EXT_MODE		AR7240_DDR_CTL_BASE+0x0c
#define AR7240_DDR_CONTROL		AR7240_DDR_CTL_BASE+0x10
#define AR7240_DDR_REFRESH		AR7240_DDR_CTL_BASE+0x14
#define AR7240_DDR_RD_DATA_THIS_CYCLE	AR7240_DDR_CTL_BASE+0x18
#define AR7240_DDR_TAP_CONTROL0		AR7240_DDR_CTL_BASE+0x1c
#define AR7240_DDR_TAP_CONTROL1		AR7240_DDR_CTL_BASE+0x20
#define AR7240_DDR_TAP_CONTROL2		AR7240_DDR_CTL_BASE+0x24
#define AR7240_DDR_TAP_CONTROL3		AR7240_DDR_CTL_BASE+0x28
#define AR7240_DDR_DDR2_CONFIG		AR7240_DDR_CTL_BASE+0x8c
#define AR7240_DDR_BURST		AR7240_DDR_CTL_BASE+0xc4
#define AR7240_DDR_BURST2		AR7240_DDR_CTL_BASE+0xc8
#define AR7240_AHB_MASTER_TIMEOUT	AR7240_DDR_CTL_BASE+0xcc
#define AR7240_DDR_CTL_CONFIG		AR7240_DDR_CTL_BASE+0x108
#define AR7240_DDR_DEBUG_RD_CNTL	AR7240_DDR_CTL_BASE+0x118

#define AR934X_CPU_PLL_DITHER		AR7240_PLL_BASE+0x0048

#define AR934X_CPU_PLL_CONFIG		AR7240_PLL_BASE+0x0000
#define AR934X_DDR_PLL_CONFIG		AR7240_PLL_BASE+0x0004
#define AR934X_CPU_DDR_CLOCK_CONTROL	AR7240_PLL_BASE+0x0008
#define AR934X_DDR_PLL_DITHER		AR7240_PLL_BASE+0x0044

#define CPU_DPLL3_ADDRESS		(KSEG1 | 0x181161c8)
#define CPU_DPLL4_ADDRESS		(KSEG1 | 0x181161cc)
#define DDR_DPLL3_ADDRESS		(KSEG1 | 0x18116248)
#define DDR_DPLL4_ADDRESS		(KSEG1 | 0x1811624c)

#define DPLL2_ADDRESS_c4		(KSEG1 | 0x181161c4)
#define DPLL2_ADDRESS_44		(KSEG1 | 0x18116244)
#define DPLL3_ADDRESS_88		(KSEG1 | 0x18116188)

#define CPU_PLL_CONFIG_NINT_VAL_40	0x380
#define DDR_PLL_CONFIG_NINT_VAL_40	0x3000
#define CPU_PLL_NFRAC_40		0
#define DDR_PLL_NFRAC_40		0

#define CPU_PLL_CONFIG_NINT_VAL_25	0x580
#define DDR_PLL_CONFIG_NINT_VAL_25	0x4c00
#define CPU_PLL_NFRAC_25		0x659
#define DDR_PLL_NFRAC_25		0x330cc

#define CPU_PLL_DITHER_DITHER_EN_LSB		31
#define CPU_PLL_DITHER_DITHER_EN_MASK		0x80000000
#define CPU_PLL_DITHER_DITHER_EN_SET(x)	\
	(((x) << CPU_PLL_DITHER_DITHER_EN_LSB) \
	  & CPU_PLL_DITHER_DITHER_EN_MASK)

#define CPU_PLL_DITHER_NFRAC_STEP_LSB		12
#define CPU_PLL_DITHER_NFRAC_STEP_MASK		0x0003f000
#define CPU_PLL_DITHER_NFRAC_STEP_SET(x) \
	(((x) << CPU_PLL_DITHER_NFRAC_STEP_LSB) \
	  & CPU_PLL_DITHER_NFRAC_STEP_MASK)

#define CPU_PLL_DITHER_UPDATE_COUNT_LSB		18
#define CPU_PLL_DITHER_UPDATE_COUNT_MASK	0x00fc0000
#define CPU_PLL_DITHER_UPDATE_COUNT_SET(x) \
	(((x) << CPU_PLL_DITHER_UPDATE_COUNT_LSB) \
	  & CPU_PLL_DITHER_UPDATE_COUNT_MASK)

#define DDR_PLL_DITHER_DITHER_EN_LSB		31
#define DDR_PLL_DITHER_DITHER_EN_MASK		0x80000000
#define DDR_PLL_DITHER_DITHER_EN_SET(x)	\
	(((x) << DDR_PLL_DITHER_DITHER_EN_LSB) \
	  & DDR_PLL_DITHER_DITHER_EN_MASK)

#define DDR_PLL_DITHER_NFRAC_STEP_LSB		20
#define DDR_PLL_DITHER_NFRAC_STEP_MASK		0x07f00000
#define DDR_PLL_DITHER_NFRAC_STEP_SET(x) \
	(((x) << DDR_PLL_DITHER_NFRAC_STEP_LSB) \
	  & DDR_PLL_DITHER_NFRAC_STEP_MASK)

#define DDR_PLL_DITHER_UPDATE_COUNT_LSB		27
#define DDR_PLL_DITHER_UPDATE_COUNT_MASK	0x78000000
#define DDR_PLL_DITHER_UPDATE_COUNT_SET(x) \
	(((x) << DDR_PLL_DITHER_UPDATE_COUNT_LSB) \
	  & DDR_PLL_DITHER_UPDATE_COUNT_MASK)

#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_LSB	2
#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK	0x00000004
#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(x) \
	 (((x) << CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_LSB) \
	   & CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK)

#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_LSB	3
#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK	0x00000008
#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(x) \
	(((x) << CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_LSB) \
	  & CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK)

#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_LSB	4
#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK	0x00000010
#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(x) \
	(((x) << CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_LSB) \
	  & CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK)

#define CPU_DPLL3_DO_MEAS_LSB			30
#define CPU_DPLL3_DO_MEAS_MASK			0x40000000
#define CPU_DPLL3_DO_MEAS_SET(x) \
	(((x) << CPU_DPLL3_DO_MEAS_LSB) & CPU_DPLL3_DO_MEAS_MASK)

#define CPU_DPLL3_SQSUM_DVC_LSB			3
#define CPU_DPLL3_SQSUM_DVC_MASK		0x007ffff8
#define CPU_DPLL3_SQSUM_DVC_SET(x) \
	(((x) << CPU_DPLL3_SQSUM_DVC_LSB) & CPU_DPLL3_SQSUM_DVC_MASK)

#define CPU_DPLL4_MEAS_DONE_LSB			3
#define CPU_DPLL4_MEAS_DONE_MASK		0x00000008
#define CPU_DPLL4_MEAS_DONE_SET(x) \
	(((x) << CPU_DPLL4_MEAS_DONE_LSB) & CPU_DPLL4_MEAS_DONE_MASK)

#define CPU_PLL_CONFIG_PLLPWD_LSB		30
#define CPU_PLL_CONFIG_PLLPWD_MASK		0x40000000
#define CPU_PLL_CONFIG_PLLPWD_SET(x) \
	(((x) << CPU_PLL_CONFIG_PLLPWD_LSB) & CPU_PLL_CONFIG_PLLPWD_MASK)

#define DDR_DPLL3_DO_MEAS_LSB			30
#define DDR_DPLL3_DO_MEAS_MASK			0x40000000
#define DDR_DPLL3_DO_MEAS_SET(x) \
	(((x) << DDR_DPLL3_DO_MEAS_LSB) & DDR_DPLL3_DO_MEAS_MASK)

#define DDR_DPLL3_SQSUM_DVC_LSB			3
#define DDR_DPLL3_SQSUM_DVC_MASK		0x007ffff8
#define DDR_DPLL3_SQSUM_DVC_SET(x) \
	(((x) << DDR_DPLL3_SQSUM_DVC_LSB) & DDR_DPLL3_SQSUM_DVC_MASK)

#define DDR_DPLL4_MEAS_DONE_LSB			3
#define DDR_DPLL4_MEAS_DONE_MASK		0x00000008
#define DDR_DPLL4_MEAS_DONE_SET(x) \
	(((x) << DDR_DPLL4_MEAS_DONE_LSB) & DDR_DPLL4_MEAS_DONE_MASK)

#define DDR_PLL_CONFIG_PLLPWD_LSB		30
#define DDR_PLL_CONFIG_PLLPWD_MASK		0x40000000
#define DDR_PLL_CONFIG_PLLPWD_SET(x) \
	(((x) << DDR_PLL_CONFIG_PLLPWD_LSB) & DDR_PLL_CONFIG_PLLPWD_MASK)

/*
 * Helper macros.
 * These Clobber t7, t8 and t9
 */
#define set_val(_reg, _mask, _val)		\
	li	t7,	KSEG1ADDR(_reg);	\
	lw	t8,	0(t7);			\
	li	t9,	~_mask;			\
	and	t8,	t8,	t9;		\
	li	t9,	_val;			\
	or	t8,	t8,	t9;		\
	sw	t8,	0(t7)

#define cpu_ddr_control_set(_mask, _val)	\
	set_val(AR934X_CPU_DDR_CLOCK_CONTROL, _mask, _val)

#define set_srif_pll_reg(reg, _r)	\
	li	t7,	KSEG1ADDR(reg);	\
	sw	_r,	0(t7);

#define inc_loop_count(loc)		\
	li	t9,	loc;		\
	lw	t7,	0(t9);		\
	addi	t7,	t7,	1;	\
	sw	t7,	0(t9);

#define clear_loop_count(loc)	\
	li	t9,	loc;	\
	sw	zero,	0(t9);

/******************************************************************************
 * first level initialization:
 *
 * 0) If clock cntrl reset switch is already set, we're recovering from
 *	"divider reset"; goto 3.
 * 1) Setup divide ratios.
 * 2) Reset.
 * 3) Setup pll's, wait for lock.
 *
 *****************************************************************************/

.macro	pbl_ar9344_v11_pll_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x13210f00, DPLL2_ADDRESS_c4
	pbl_reg_writel	0x03000000, CPU_DPLL3_ADDRESS
	pbl_reg_writel	0x13210f00, DPLL2_ADDRESS_44
	pbl_reg_writel	0x03000000, DDR_DPLL3_ADDRESS
	pbl_reg_writel	0x03000000, DPLL3_ADDRESS_88

	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	WASP_REF_CLK_25
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_ref25_val
	nop

setup_ref40_val:
	li	t5,	CPU_PLL_CONFIG_NINT_VAL_40
	li	t6,	DDR_PLL_CONFIG_NINT_VAL_40
	li	t7,	CPU_PLL_NFRAC_40
	li	t9,	DDR_PLL_NFRAC_40
	b	1f
	nop

setup_ref25_val:
	li	t5,	CPU_PLL_CONFIG_NINT_VAL_25
	li	t6,	DDR_PLL_CONFIG_NINT_VAL_25
	li	t7,	CPU_PLL_NFRAC_25
	li	t9,	DDR_PLL_NFRAC_25

1:
	li	t4,	(CPU_PLL_DITHER_DITHER_EN_SET(0) | \
			CPU_PLL_DITHER_NFRAC_STEP_SET(1) | \
			CPU_PLL_DITHER_UPDATE_COUNT_SET(0xf));
	or	t4,	t4,	t7

	li	t8,	0x21000;
	or	t5,	t5,	t8

	li	t8,	0x210000;
	or	t6,	t6,	t8

	li	t3,	(DDR_PLL_DITHER_DITHER_EN_SET(0) | \
			DDR_PLL_DITHER_NFRAC_STEP_SET(1) | \
			DDR_PLL_DITHER_UPDATE_COUNT_SET(0xf));

	or	t3,	t3,	t9

pll_bypass_set:
	/* reg, mask, val  */
	/* 0xb8050008, 0xfffffffb, 0x4 */
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(1));
	/* 0xb8050008, 0xfffffff7, 0x8 */
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(1));
	/* 0xb8050008, 0xffffffef, 0x10 */
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(1));

init_cpu_pll:
	li	t7,	AR934X_CPU_PLL_CONFIG
	li	t8,	CPU_PLL_CONFIG_PLLPWD_SET(1)
	or	t8,	t8,	t5
	sw	t8,	0(t7);

init_ddr_pll:
	li	t7,	AR934X_DDR_PLL_CONFIG
	li	t8,	DDR_PLL_CONFIG_PLLPWD_SET(1)
	or	t8,	t8,	t6
	sw	t8,	0(t7);

init_ahb_pll:
	pbl_reg_writel	0x0130801C, AR934X_CPU_DDR_CLOCK_CONTROL

srif_set:
	/* Use built in values, based on ref clock */
	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	WASP_REF_CLK_25
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	/* jump to 25ref clk */
	beq	zero,	t6,	1f
	nop

	/*		refdiv		nint		nfrac */
	/* cpu freq = (40 MHz refclk/refdiv 8) * Nint */
	li	t4,	((0x8 << 27) | (112 << 18) | 0);
	/* ddr freq = (40 MHz refclk/refdiv 8) * Nint */
	li	t5,	((0x8 << 27) | (90 << 18) | 0);
	b	2f
	nop
1:

	/* cpu freq = (25 MHz refclk/refdiv 5) * Nint */
	li	t4,	((0x5 << 27) | (112 << 18) | 0);
	/* ddr freq = (25 MHz refclk/refdiv 5) * Nint */
	li	t5,	((0x5 << 27) | (90 << 18) | 0);

2:

	/* 0 to 0xbd000004 */
	clear_loop_count(ATH_CPU_COUNT_LOC);

cpu_pll_is_not_locked:
	inc_loop_count(ATH_CPU_COUNT_LOC);

	pbl_reg_writel 0x10810F00, DPLL2_ADDRESS_c4

	set_srif_pll_reg(0xb81161c0, t4);

	pbl_reg_writel 0xd0810f00, DPLL2_ADDRESS_c4
	pbl_reg_writel 0x03000000, CPU_DPLL3_ADDRESS
	pbl_reg_writel 0xd0800f00, DPLL2_ADDRESS_c4

cpu_clear_do_meas1:
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~CPU_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	sw	t8,	0(t7)

cpu_set_do_meas:
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	CPU_DPLL3_DO_MEAS_SET(1)
	or	t8,	t8,	t9
	sw	t8,	0(t7)

	li	t7,	KSEG1ADDR(CPU_DPLL4_ADDRESS)
cpu_wait_for_meas_done:
	lw	t8,	0(t7)
	andi	t8,	t8,	CPU_DPLL4_MEAS_DONE_SET(1)
	beqz	t8,	cpu_wait_for_meas_done
	nop

cpu_clear_do_meas2:
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~CPU_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	sw	t8,	0(t7)

cpu_read_sqsum_dvc:
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	CPU_DPLL3_SQSUM_DVC_MASK
	and	t8,	t8,	t9
	sra	t8,	t8,	CPU_DPLL3_SQSUM_DVC_LSB
	li	t9,	0x40000
	subu	t8,	t8,	t9
	bgez	t8,	cpu_pll_is_not_locked
	nop

	/* DDR */
	clear_loop_count(ATH_DDR_COUNT_LOC)

ddr_pll_is_not_locked:

	inc_loop_count(ATH_DDR_COUNT_LOC)

	pbl_reg_writel 0x10810F00, DPLL2_ADDRESS_44

	set_srif_pll_reg(0xb8116240, t5);

	pbl_reg_writel 0xD0810F00, DPLL2_ADDRESS_44
	pbl_reg_writel 0x03000000, DDR_DPLL3_ADDRESS
	pbl_reg_writel 0xD0800F00, DPLL2_ADDRESS_44

ddr_clear_do_meas1:
	li	t7,	DDR_DPLL3_ADDRESS
	lw	t8,	0(t7)
	li	t9,	~DDR_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	sw	t8,	0(t7)


ddr_set_do_meas:
	li	t7,	DDR_DPLL3_ADDRESS
	lw	t8,	0(t7)
	li	t9,	DDR_DPLL3_DO_MEAS_SET(1)
	or	t8,	t8,	t9
	sw	t8,	0(t7)

	li	t7,	KSEG1ADDR(DDR_DPLL4_ADDRESS)
ddr_wait_for_meas_done:
	lw	t8,	0(t7)
	andi	t8,	t8,	DDR_DPLL4_MEAS_DONE_SET(1)
	beqz	t8,	ddr_wait_for_meas_done
	nop

ddr_clear_do_meas2:
	li	t7,	DDR_DPLL3_ADDRESS
	lw	t8,	0(t7)
	li	t9,	~DDR_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	sw	t8,	0(t7)

ddr_read_sqsum_dvc:
	li	t7,	DDR_DPLL3_ADDRESS
	lw	t8,	0(t7)
	li	t9,	DDR_DPLL3_SQSUM_DVC_MASK
	and	t8,	t8,	t9
	sra	t8,	t8,	DDR_DPLL3_SQSUM_DVC_LSB
	li	t9,	0x40000
	subu	t8,	t8,	t9
	bgez	t8,	ddr_pll_is_not_locked
	nop

pll_bypass_unset:
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(0));
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(0));
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, \
		CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(0));

ddr_pll_dither_unset:
	pbl_reg_writel	0x78180200, AR934X_DDR_PLL_DITHER

cpu_pll_dither_unset:
	li	t7,	AR934X_CPU_PLL_DITHER
	sw	t4,	0(t7)

	.set	pop
.endm

#define AR9344_DDR_DDR2_CONFIG			AR7240_DDR_CTL_BASE+0xb8
#define CFG_934X_DDR2_EN_TWL_VAL		0x0e59
#define USEC_MULT				1
#define CFG_934X_DDR2_CONFIG_VAL		0xc7d48cd0
#define CFG_934X_DDR2_CONFIG2_VAL		0x9dd0e6a8
#define CFG_934X_DDR2_MODE_VAL_INIT		0x133
#define CFG_934X_DDR2_EXT_MODE_VAL_INIT		0x382
#define CFG_934X_DDR2_EXT_MODE_VAL		0x402
#define CFG_934X_DDR2_MODE_VAL			0x33
#define CFG_DDR_REFRESH_VAL			0x4270
#define CFG_934X_DDR2_TAP_VAL			0x10012
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32	0xff
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16	0xffff

.macro	pbl_ar9344_v11_ddr2_config
	.set	push
	.set	noreorder

	pbl_reg_writel	CFG_934X_DDR2_EN_TWL_VAL, AR9344_DDR_DDR2_CONFIG
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x10, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	0x20, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	BIT(3)
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_16bit_1
	nop
setup_32bit_1:
	pbl_reg_writel	BIT(6), AR7240_DDR_CTL_CONFIG
	b	1f
	nop
setup_16bit_1:
	pbl_reg_clr	BIT(6), AR7240_DDR_CTL_CONFIG
1:

	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_CONFIG_VAL, AR7240_DDR_CONFIG
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_CONFIG2_VAL, AR7240_DDR_CONFIG2
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x8, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_MODE_VAL_INIT, AR7240_DDR_MODE
	pbl_sleep	t2, 1000 * USEC_MULT

	pbl_reg_writel	0x1, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_EXT_MODE_VAL_INIT, AR7240_DDR_EXT_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x2, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_EXT_MODE_VAL, AR7240_DDR_EXT_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x2, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	0x8, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_MODE_VAL, AR7240_DDR_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x1, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	pbl_reg_writel	CFG_DDR_REFRESH_VAL, AR7240_DDR_REFRESH
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL0
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL1


	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	BIT(3)
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_16bit_2
	nop
setup_32bit_2:
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL2
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL3
	pbl_reg_writel	CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32, AR7240_DDR_RD_DATA_THIS_CYCLE
	b	1f
	nop
setup_16bit_2:
	pbl_reg_writel	CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16, AR7240_DDR_RD_DATA_THIS_CYCLE

1:
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x74444444, AR7240_DDR_BURST
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel	0x222, AR7240_DDR_BURST2
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel 0xfffff, AR7240_AHB_MASTER_TIMEOUT
	pbl_sleep	t2, 100 * USEC_MULT

	.set	pop
.endm

#endif /* __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H */
