#include <config.h>
#include <mach/imx-regs.h>
#include <mach/imx-pll.h>
#include <linux/linkage.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define CSCR_VAL CSCR_USB_DIV(3) |	\
		 CSCR_SD_CNT(3) |	\
		 CSCR_MSHC_SEL |	\
		 CSCR_H264_SEL |	\
		 CSCR_SSI1_SEL |	\
		 CSCR_SSI2_SEL |	\
		 CSCR_MCU_SEL |		\
		 CSCR_ARM_SRC_MPLL |	\
		 CSCR_SP_SEL |		\
		 CSCR_ARM_DIV(0) |	\
		 CSCR_FPM_EN |		\
		 CSCR_SPEN |		\
		 CSCR_MPEN |		\
		 CSCR_AHB_DIV(1)

ENTRY(neso_pll_init)

	writel(IMX_PLL_PD(0) |
		 IMX_PLL_MFD(51) |
		 IMX_PLL_MFI(7) |
		 IMX_PLL_MFN(35), MPCTL0) /* 399 MHz */

	writel(IMX_PLL_PD(1) |
		 IMX_PLL_MFD(12) |
		 IMX_PLL_MFI(9) |
		 IMX_PLL_MFN(3), SPCTL0) /* SPLL = 2 * 26 * 4.61538 MHz = 240 MHz */

	writel(CSCR_VAL | CSCR_MPLL_RESTART | CSCR_SPLL_RESTART, CSCR)

	ldr r2, =16000
1:
	subs    r2, r2, #1
	nop
	bcs 1b

	mov     pc, lr
ENDPROC(neso_pll_init)

