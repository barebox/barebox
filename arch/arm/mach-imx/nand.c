/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <mach/imx-regs.h>
#include <asm/io.h>

#if defined(CONFIG_ARCH_IMX35) || defined (CONFIG_ARCH_IMX25)

#define RCSR_NFC_FMS		(1 << 8)
#define RCSR_NFC_4K		(1 << 9)
#define RCSR_NFC_16BIT_SEL	(1 << 14)

void imx_nand_set_layout(int writesize, int datawidth)
{
	unsigned int rcsr;

	rcsr = readl(IMX_CCM_BASE + CCM_RCSR);

	switch (writesize) {
	case 512:
		rcsr &= ~(RCSR_NFC_FMS | RCSR_NFC_4K);
		break;
	case 2048:
		rcsr |= RCSR_NFC_FMS;
		break;
	case 4096:
		rcsr |= RCSR_NFC_FMS | RCSR_NFC_4K;
		break;
	default:
		break;
	}

	switch (datawidth) {
	case 8:
		rcsr &= ~RCSR_NFC_16BIT_SEL;
		break;
	case 16:
		rcsr |= RCSR_NFC_16BIT_SEL;
		break;
	default:
		break;
	}

	writel(rcsr, IMX_CCM_BASE + CCM_RCSR);
}

#elif defined(CONFIG_ARCH_IMX21) || defined (CONFIG_ARCH_IMX27)

#define FMCR_NF_FMS		(1 << 5)
#define FMCR_NF_16BIT_SEL	(1 << 4)

void imx_nand_set_layout(int writesize, int datawidth)
{
	unsigned int fmcr;

	fmcr = FMCR;

	switch (writesize) {
	case 512:
		fmcr &= ~FMCR_NF_FMS;
		break;
	case 2048:
		fmcr |= FMCR_NF_FMS;
		break;
	default:
		break;
	}

	switch (datawidth) {
	case 8:
		fmcr &= ~FMCR_NF_16BIT_SEL;
		break;
	case 16:
		fmcr |= FMCR_NF_16BIT_SEL;
		break;
	default:
		break;
	}

	FMCR = fmcr;
}

#elif defined CONFIG_ARCH_IMX51

void imx_nand_set_layout(int writesize, int datawidth)
{
	/* Just silence the compiler warning below. On i.MX51 we don't
	 * have external boot.
	 */
}

#else
#warning using empty imx_nand_set_layout(). NAND flash will not work properly if not booting from it

void imx_nand_set_layout(int writesize, int datawidth)
{
}

#endif
