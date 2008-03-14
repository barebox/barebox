/*
 * For clock initialization, see chapter 3 of the "MCIMX27 Multimedia
 * Applications Processor Reference Manual, Rev. 0.2".
 *
 */

#include <config.h>
#include <asm/arch/imx-regs.h>

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
	writel(0x00000000, 0xA0000F00)	/* run auto-refresh cycle to array 0 */
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE, ESDCTL0)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0xff
	strb		r1, [r0]
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 | ESDCTL0_BL | ESDCTL0_SMODE_NORMAL, ESDCTL0)
.endm

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r10, lr

	/* ahb lite ip interface */
	writel(0x20040304, AIPI1_PSR0)
	writel(0xDFFBFCFB, AIPI1_PSR1)
	writel(0x00000000, AIPI2_PSR0)
	writel(0xFFFFFFFF, AIPI2_PSR1)

	/* disable mpll/spll */
	ldr r0, =CSCR
	ldr r1, [r0]
	bic r1, r1, #0x03
	str r1, [r0]

	/*
	 * pll clock initialization - see section 3.4.3 of the i.MX27 manual
	 */
	writel(PLL_PCTL_PD(1) |		\
		PLL_PCTL_MFD(51) |	\
		PLL_PCTL_MFI(7) |	\
		PLL_PCTL_MFN(35),	\
		MPCTL0)	              /* MPLL = 2 * 26 * 3.83654 MHz = 199.5 MHz */

	writel(PLL_PCTL_PD(1) |		\
		PLL_PCTL_MFD(12) |	\
		PLL_PCTL_MFI(9) |	\
		PLL_PCTL_MFN(3),	\
		SPCTL0)	              /* SPLL = 2 * 26 * 4.61538 MHz = 240 MHz */

	/*
	 * ARM clock = (399 MHz / 2) / (ARM divider = 1) = 200 MHz
	 * AHB clock = (399 MHz / 3) / (AHB divider = 2) = 66.5 MHz
	 * System clock (HCLK) = 133 MHz
	 */
#define CSCR_VAL CSCR_USB_DIV(3) |	\
		 CSCR_SD_CNT(3) |	\
		 CSCR_MSHC_SEL |	\
		 CSCR_H264_SEL |	\
		 CSCR_SSI1_SEL |	\
		 CSCR_SSI2_SEL |	\
		 CSCR_MCU_SEL |		\
		 CSCR_SP_SEL |		\
		 CSCR_ARM_SRC_MPLL |	\
		 CSCR_AHB_DIV(1) |	\
		 CSCR_ARM_DIV(0) |	\
		 CSCR_FPM_EN |		\
		 CSCR_SPEN |		\
		 CSCR_MPEN

	writel(CSCR_VAL | CSCR_MPLL_RESTART | CSCR_SPLL_RESTART, CSCR)

	/* add some delay here */
	mov r1, #0x8000
1:	subs r1, r1, #0x1
	bne 1b

	/* clock gating enable */
	writel(0x00050f08, GPCR)

	/* peripheral clock divider */
	writel(0x130410c3, PCDR0)	/* FIXME                            */
	writel(0x09030908, PCDR1)	/* PERDIV1=08 @133 MHz              */
					/* PERDIV1=04 @266 MHz              */

	/* configure 16 bit nor flash on cs0 */
	writel(0x0000CC03, 0xD8002000)
	writel(0xa0330D01, 0xD8002004)
	writel(0x00220800, 0xD8002008)

	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	mov	pc,r10
1:
	sdram_init

	mov	pc,r10

