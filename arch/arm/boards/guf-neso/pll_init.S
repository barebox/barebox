#include <config.h>
#include <mach/imx27-regs.h>
#include <mach/imx-pll.h>
#include <linux/linkage.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define CSCR_VAL MX27_CSCR_USB_DIV(3) |	\
		 MX27_CSCR_SD_CNT(3) |	\
		 MX27_CSCR_MSHC_SEL |	\
		 MX27_CSCR_H264_SEL |	\
		 MX27_CSCR_SSI1_SEL |	\
		 MX27_CSCR_SSI2_SEL |	\
		 MX27_CSCR_MCU_SEL |		\
		 MX27_CSCR_ARM_SRC_MPLL |	\
		 MX27_CSCR_SP_SEL |		\
		 MX27_CSCR_ARM_DIV(0) |	\
		 MX27_CSCR_FPM_EN |		\
		 MX27_CSCR_SPEN |		\
		 MX27_CSCR_MPEN |		\
		 MX27_CSCR_AHB_DIV(1)

ENTRY(neso_pll_init)

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

	writel(CSCR_VAL | MX27_CSCR_MPLL_RESTART | MX27_CSCR_SPLL_RESTART,
			MX27_CCM_BASE_ADDR + MX27_CSCR)

	ldr r2, =16000
1:
	subs    r2, r2, #1
	nop
	bcs 1b

	mov     pc, lr
ENDPROC(neso_pll_init)

