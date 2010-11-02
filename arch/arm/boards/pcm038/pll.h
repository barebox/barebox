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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * @file
 * @brief phyCORE-i.MX27 specific PLL setup
 */

#ifndef __PCM038_PLL_H
#define __PCM038_PLL_H

/* define the PLL setting we want to run the system  */

/* main clock divider settings immediately after reset (at 1.25 V core supply) */
#define CSCR_VAL (CSCR_USB_DIV(3) |	\
		CSCR_SD_CNT(3) |	\
		CSCR_MSHC_SEL |		\
		CSCR_H264_SEL |		\
		CSCR_SSI1_SEL |		\
		CSCR_SSI2_SEL |		\
		CSCR_SP_SEL | /* 26 MHz reference */ \
		CSCR_MCU_SEL | /* 26 MHz reference */ \
		CSCR_ARM_DIV(0) | /* CPU runs at MPLL/3 clock */ \
		CSCR_AHB_DIV(1) | /* AHB runs at MPLL/6 clock */ \
		CSCR_SPEN |		\
		CSCR_MPEN)

/* main clock divider settings after core voltage increases to 1.45 V */
#define CSCR_VAL_FINAL (CSCR_USB_DIV(3) |	\
		CSCR_SD_CNT(3) |	\
		CSCR_MSHC_SEL |		\
		CSCR_H264_SEL |		\
		CSCR_SSI1_SEL |		\
		CSCR_SSI2_SEL |		\
		CSCR_SP_SEL | /* 26 MHz reference */ \
		CSCR_MCU_SEL | /* 26 MHz reference */ \
		CSCR_ARM_SRC_MPLL | /* use main MPLL clock */ \
		CSCR_ARM_DIV(0) | /* CPU run at full MPLL clock */ \
		CSCR_AHB_DIV(1) | /* AHB runs at MPLL/6 clock */ \
		CSCR_SPEN |		\
		CSCR_MPEN)

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
