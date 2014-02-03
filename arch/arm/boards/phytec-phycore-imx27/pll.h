/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief phyCORE-i.MX27 specific PLL setup
 */

#ifndef __PCM038_PLL_H
#define __PCM038_PLL_H

/* define the PLL setting we want to run the system  */

/* main clock divider settings immediately after reset (at 1.25 V core supply) */
#define CSCR_VAL (MX27_CSCR_USB_DIV(3) |	\
		MX27_CSCR_SD_CNT(3) |	\
		MX27_CSCR_MSHC_SEL |		\
		MX27_CSCR_H264_SEL |		\
		MX27_CSCR_SSI1_SEL |		\
		MX27_CSCR_SSI2_SEL |		\
		MX27_CSCR_SP_SEL | /* 26 MHz reference */ \
		MX27_CSCR_MCU_SEL | /* 26 MHz reference */ \
		MX27_CSCR_ARM_DIV(0) | /* CPU runs at MPLL/3 clock */ \
		MX27_CSCR_AHB_DIV(1) | /* AHB runs at MPLL/6 clock */ \
		MX27_CSCR_FPM_EN | \
		MX27_CSCR_SPEN |		\
		MX27_CSCR_MPEN)

/* main clock divider settings after core voltage increases to 1.45 V */
#define CSCR_VAL_FINAL (MX27_CSCR_USB_DIV(3) |	\
		MX27_CSCR_SD_CNT(3) |	\
		MX27_CSCR_MSHC_SEL |		\
		MX27_CSCR_H264_SEL |		\
		MX27_CSCR_SSI1_SEL |		\
		MX27_CSCR_SSI2_SEL |		\
		MX27_CSCR_SP_SEL | /* 26 MHz reference */ \
		MX27_CSCR_MCU_SEL | /* 26 MHz reference */ \
		MX27_CSCR_ARM_SRC_MPLL | /* use main MPLL clock */ \
		MX27_CSCR_ARM_DIV(0) | /* CPU run at full MPLL clock */ \
		MX27_CSCR_AHB_DIV(1) | /* AHB runs at MPLL/6 clock */ \
		MX27_CSCR_FPM_EN | /* do not disable it! */ \
		MX27_CSCR_SPEN |		\
		MX27_CSCR_MPEN)

/* MPLL should provide a 399 MHz clock from the 26 MHz reference */
#define MPCTL0_VAL (IMX_PLL_PD(0) |	\
		IMX_PLL_MFD(51) |	\
		IMX_PLL_MFI(7) |	\
		IMX_PLL_MFN(35))

/* SPLL should provide a 240 MHz clock from the 26 MHz reference */
#define SPCTL0_VAL (IMX_PLL_PD(1) |	\
		IMX_PLL_MFD(12) |	\
		IMX_PLL_MFI(9) |	\
		IMX_PLL_MFN(3))


#endif	/* __PCM038_PLL_H */
