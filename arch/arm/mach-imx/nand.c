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
 */

#include <common.h>
#include <mach/generic.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx35-regs.h>
#include <mach/imx-nand.h>
#include <io.h>

#define RCSR_NFC_FMS		(1 << 8)
#define RCSR_NFC_4K		(1 << 9)
#define RCSR_NFC_16BIT_SEL	(1 << 14)

static __maybe_unused void imx25_35_nand_set_layout(void __iomem *reg_rcsr,
		int writesize, int datawidth)
{
	unsigned int rcsr;

	rcsr = readl(reg_rcsr);

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

	writel(rcsr, reg_rcsr);
}

#define FMCR_NF_FMS		(1 << 5)
#define FMCR_NF_16BIT_SEL	(1 << 4)

static __maybe_unused void imx21_27_nand_set_layout(void __iomem *reg_fmcr,
		int writesize, int datawidth)
{
	unsigned int fmcr;

	fmcr = readl(reg_fmcr);

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

	writel(fmcr, reg_fmcr);
}

void imx_nand_set_layout(int writesize, int datawidth)
{
#ifdef CONFIG_ARCH_IMX21
	if (cpu_is_mx21())
		imx21_27_nand_set_layout((void *)(MX21_SYSCTRL_BASE_ADDR +
					0x14), writesize, datawidth);
#endif
#ifdef CONFIG_ARCH_IMX27
	if (cpu_is_mx27())
		imx21_27_nand_set_layout((void *)(MX27_SYSCTRL_BASE_ADDR +
					0x14), writesize, datawidth);
#endif
#ifdef CONFIG_ARCH_IMX25
	if (cpu_is_mx25())
		imx25_35_nand_set_layout((void *)MX25_CCM_BASE_ADDR +
				MX25_CCM_RCSR, writesize, datawidth);
#endif
#ifdef CONFIG_ARCH_IMX35
	if (cpu_is_mx35())
		imx25_35_nand_set_layout((void *)MX35_CCM_BASE_ADDR +
				MX35_CCM_RCSR, writesize, datawidth);
#endif
}
