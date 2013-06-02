/*
 * For clock initialization, see chapter 3 of the "MCIMX27 Multimedia
 * Applications Processor Reference Manual, Rev. 0.2".
 *
 */

#include <config.h>
#include <mach/imx27-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];


#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

.macro sdram_init
	/*
	 * DDR on CSD0
	 */
	/* Enable DDR SDRAM operation */
	writel(0x00000008, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC)

	/* Set the driving strength   */
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(3))
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(5))
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(6))
	writel(0x00005005, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(7))
	writel(0x15555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(8))

	/* Initial reset */
	writel(0x00000004, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC)
	writel(0x006ac73a, MX27_ESDCTL_BASE_ADDR + IMX_ESDCFG0)

	/* precharge CSD0 all banks */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
	writel(0x00000000, 0xA0000F00)	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)

	ldr	r0, =0xa0000f00
	mov	r1, #0
	mov	r2, #8
1:
	str	r1, [r0]
	subs	r2, #1
	bne	1b

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0xff
	strb		r1, [r0]
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 |
			ESDCTL0_BL | ESDCTL0_SMODE_NORMAL,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
.endm

	.section ".text_bare_init","ax"

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
	bl		arm_cpu_lowlevel_init

	/* ahb lite ip interface */
	writel(0x20040304, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR0)
	writel(0xDFFBFCFB, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR1)
	writel(0x00000000, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR0)
	writel(0xFFFFFFFF, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR1)

	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	b	imx27_barebox_entry

1:
	/* 399 MHz */
	writel(IMX_PLL_PD(0) |
		 IMX_PLL_MFD(51) |
		 IMX_PLL_MFI(7) |
		 IMX_PLL_MFN(35), MX27_CCM_BASE_ADDR + MX27_MPCTL0)

	/* SPLL = 2 * 26 * 4.61538 MHz = 240 MHz */
	writel(IMX_PLL_PD(1) |
		 IMX_PLL_MFD(12) |
		 IMX_PLL_MFI(9) |
		 IMX_PLL_MFN(3), MX27_CCM_BASE_ADDR + MX27_SPCTL0)

	writel(MX27_CSCR_MPLL_RESTART | MX27_CSCR_SPLL_RESTART |
		MX27_CSCR_ARM_SRC_MPLL | MX27_CSCR_MCU_SEL |
		MX27_CSCR_SP_SEL | MX27_CSCR_FPM_EN |
		MX27_CSCR_MPEN | MX27_CSCR_SPEN | MX27_CSCR_ARM_DIV(0) |
		MX27_CSCR_AHB_DIV(1) | MX27_CSCR_USB_DIV(3) |
		MX27_CSCR_SD_CNT(3) | MX27_CSCR_SSI2_SEL |
		MX27_CSCR_SSI1_SEL | MX27_CSCR_H264_SEL |
		MX27_CSCR_MSHC_SEL, MX27_CCM_BASE_ADDR + MX27_CSCR)

	sdram_init

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =0xa0f00000		/* Setup a temporary stack in SDRAM */

	b	imx27_barebox_boot_nand_external
#endif /* CONFIG_NAND_IMX_BOOT */

ret:
	b	imx27_barebox_entry

