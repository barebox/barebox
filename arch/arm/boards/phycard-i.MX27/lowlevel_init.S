/*
 * For clock initialization, see chapter 3 of the "MCIMX27 Multimedia
 * Applications Processor Reference Manual, Rev. 0.2".
 *
 */

#include <config.h>
#include <mach/imx-regs.h>
#include <mach/imx-pll.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];


#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

.macro sdram_init
	/*
	 * DDR on CSD0
	 */
	writel(0x00000008, ESDMISC) /* Enable DDR SDRAM operation */

	writel(0x55555555, DSCR(3)) /* Set the driving strength   */
	writel(0x55555555, DSCR(5))
	writel(0x55555555, DSCR(6))
	writel(0x00005005, DSCR(7))
	writel(0x15555555, DSCR(8))

	writel(0x00000004, ESDMISC) /* Initial reset */
	writel(0x006ac73a, ESDCFG0)

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE, ESDCTL0) /* precharge CSD0 all banks */
	writel(0x00000000, 0xA0000F00)	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH, ESDCTL0)

	ldr	r0, =0xa0000f00
	mov	r1, #0
	mov	r2, #8
1:
	str	r1, [r0]
	subs	r2, #1
	bne	1b

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE, ESDCTL0)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0xff
	strb		r1, [r0]
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 | ESDCTL0_BL | ESDCTL0_SMODE_NORMAL, ESDCTL0)
.endm

	.section ".text_bare_init","ax"

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r10, lr

	/* ahb lite ip interface */
	writel(0x20040304, AIPI1_PSR0)
	writel(0xDFFBFCFB, AIPI1_PSR1)
	writel(0x00000000, AIPI2_PSR0)
	writel(0xFFFFFFFF, AIPI2_PSR1)

	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	mov	pc,r10

1:
	writel(IMX_PLL_PD(0) |
		 IMX_PLL_MFD(51) |
		 IMX_PLL_MFI(7) |
		 IMX_PLL_MFN(35), MPCTL0) /* 399 MHz */

	writel(IMX_PLL_PD(1) |
		 IMX_PLL_MFD(12) |
		 IMX_PLL_MFI(9) |
		 IMX_PLL_MFN(3), SPCTL0) /* SPLL = 2 * 26 * 4.61538 MHz = 240 MHz */

	writel(CSCR_MPLL_RESTART | CSCR_SPLL_RESTART | CSCR_ARM_SRC_MPLL |
	       CSCR_MCU_SEL | CSCR_SP_SEL | CSCR_FPM_EN | CSCR_MPEN |
	       CSCR_SPEN | CSCR_ARM_DIV(0) | CSCR_AHB_DIV(1) | CSCR_USB_DIV(3) |
	       CSCR_SD_CNT(3) | CSCR_SSI2_SEL | CSCR_SSI1_SEL | CSCR_H264_SEL |
	       CSCR_MSHC_SEL, CSCR)

	sdram_init

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =0xa0f00000		/* Setup a temporary stack in SDRAM */

	ldr	r0, =IMX_NFC_BASE		/* start of NFC SRAM                */
	ldr	r2, =IMX_NFC_BASE + 0x1000	/* end of NFC SRAM                  */

	/* skip NAND boot if not running from NFC space */
	cmp	pc, r0
	bls	ret
	cmp	pc, r2
	bhi	ret

	/* Move ourselves out of NFC SRAM */
	ldr	r1, =_text

copy_loop:
	ldmia	r0!, {r3-r9}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r9}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
	ble	copy_loop

	ldr	pc, =1f			/* Jump to SDRAM                    */
1:
	b	nand_boot		/* Load barebox from NAND Flash      */

#endif /* CONFIG_NAND_IMX_BOOT */

ret:
	mov	pc,r10

