/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 * Copyright 2008 Martin Fuzzey, mfuzzey@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx21-regs.h>

#include "clk.h"

/* Register offsets */
#define CCM_CSCR		0x0
#define CCM_MPCTL0		0x4
#define CCM_MPCTL1		0x8
#define CCM_SPCTL0		0xc
#define CCM_SPCTL1		0x10
#define CCM_OSC26MCTL		0x14
#define CCM_PCDR0		0x18
#define CCM_PCDR1		0x1c
#define CCM_PCCR0		0x20
#define CCM_PCCR1		0x24
#define CCM_CCSR		0x28
#define CCM_PMCTL		0x2c
#define CCM_PMCOUNT		0x30
#define CCM_WKGDCTL		0x34

#define PCCR0_UART1_EN		(1 << 0)
#define PCCR0_UART2_EN		(1 << 1)
#define PCCR0_UART3_EN		(1 << 2)
#define PCCR0_UART4_EN		(1 << 3)
#define PCCR0_CSPI1_EN		(1 << 4)
#define PCCR0_CSPI2_EN		(1 << 5)
#define PCCR0_SSI1_EN		(1 << 6)
#define PCCR0_SSI2_EN		(1 << 7)
#define PCCR0_FIRI_EN		(1 << 8)
#define PCCR0_SDHC1_EN		(1 << 9)
#define PCCR0_SDHC2_EN		(1 << 10)
#define PCCR0_GPIO_EN		(1 << 11)
#define PCCR0_I2C_EN		(1 << 12)
#define PCCR0_DMA_EN		(1 << 13)
#define PCCR0_USBOTG_EN		(1 << 14)
#define PCCR0_EMMA_EN		(1 << 15)
#define PCCR0_SSI2_BAUD_EN	(1 << 16)
#define PCCR0_SSI1_BAUD_EN	(1 << 17)
#define PCCR0_PERCLK3_EN	(1 << 18)
#define PCCR0_NFC_EN		(1 << 19)
#define PCCR0_FRI_BAUD_EN	(1 << 20)
#define PCCR0_SLDC_EN		(1 << 21)
#define PCCR0_PERCLK4_EN	(1 << 22)
#define PCCR0_HCLK_BMI_EN	(1 << 23)
#define PCCR0_HCLK_USBOTG_EN	(1 << 24)
#define PCCR0_HCLK_SLCDC_EN	(1 << 25)
#define PCCR0_HCLK_LCDC_EN	(1 << 26)
#define PCCR0_HCLK_EMMA_EN	(1 << 27)
#define PCCR0_HCLK_BROM_EN	(1 << 28)
#define PCCR0_HCLK_DMA_EN	(1 << 30)
#define PCCR0_HCLK_CSI_EN	(1 << 31)

#define PCCR1_CSPI3_EN	(1 << 23)
#define PCCR1_WDT_EN	(1 << 24)
#define PCCR1_GPT1_EN	(1 << 25)
#define PCCR1_GPT2_EN	(1 << 26)
#define PCCR1_GPT3_EN	(1 << 27)
#define PCCR1_PWM_EN	(1 << 28)
#define PCCR1_RTC_EN	(1 << 29)
#define PCCR1_KPP_EN	(1 << 30)
#define PCCR1_OWIRE_EN	(1 << 31)

enum imx21_clks {
	ckil, ckih, fpm, mpll_sel, spll_sel, mpll, spll, fclk, hclk, ipg, per1,
	per2, per3, per4, usb_div, nfc_div, lcdc_per_gate, lcdc_ahb_gate,
	lcdc_ipg_gate, clk_max
};

static struct clk *clks[clk_max];

static const char *mpll_sel_clks[] = {
	"fpm",
	"ckih",
};

static const char *spll_sel_clks[] = {
	"fpm",
	"ckih",
};

static int imx21_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;
	unsigned long lref = 32768;
	unsigned long href = 26000000;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	writel(PCCR0_UART1_EN | PCCR0_UART2_EN | PCCR0_UART3_EN | PCCR0_UART4_EN |
			PCCR0_CSPI1_EN | PCCR0_CSPI2_EN | PCCR0_SDHC1_EN |
			PCCR0_SDHC2_EN | PCCR0_GPIO_EN | PCCR0_I2C_EN | PCCR0_DMA_EN |
			PCCR0_USBOTG_EN | PCCR0_NFC_EN | PCCR0_PERCLK4_EN |
			PCCR0_HCLK_USBOTG_EN | PCCR0_HCLK_DMA_EN,
			base + CCM_PCCR0);

	writel(PCCR1_CSPI3_EN | PCCR1_WDT_EN | PCCR1_GPT1_EN | PCCR1_GPT2_EN |
			PCCR1_GPT3_EN | PCCR1_PWM_EN | PCCR1_RTC_EN | PCCR1_KPP_EN |
			PCCR1_OWIRE_EN,
			base + CCM_PCCR1);

	clks[ckil] = clk_fixed("ckil", lref);
	clks[ckih] = clk_fixed("ckih", href);
	clks[fpm] = imx_clk_fixed_factor("fpm", "ckil", 512, 1);
	clks[mpll_sel] = imx_clk_mux("mpll_sel", base + CCM_CSCR, 16, 1, mpll_sel_clks,
			ARRAY_SIZE(mpll_sel_clks));
	clks[spll_sel] = imx_clk_mux("spll_sel", base + CCM_CSCR, 17, 1, spll_sel_clks,
			ARRAY_SIZE(spll_sel_clks));
	clks[mpll] = imx_clk_pllv1("mpll", "mpll_sel", base + CCM_MPCTL0);
	clks[spll] = imx_clk_pllv1("spll", "spll_sel", base + CCM_SPCTL0);
	clks[fclk] = imx_clk_divider("fclk", "mpll", base + CCM_CSCR, 29, 3);
	clks[hclk] = imx_clk_divider("hclk", "fclk", base + CCM_CSCR, 10, 4);
	clks[ipg] = imx_clk_divider("ipg", "hclk", base + CCM_CSCR, 9, 1);
	clks[per1] = imx_clk_divider("per1", "mpll", base + CCM_PCDR1, 0, 6);
	clks[per2] = imx_clk_divider("per2", "mpll", base + CCM_PCDR1, 8, 6);
	clks[per3] = imx_clk_divider("per3", "mpll", base + CCM_PCDR1, 16, 6);
	clks[per4] = imx_clk_divider("per4", "mpll", base + CCM_PCDR1, 24, 6);
	clks[usb_div] = imx_clk_divider("usb_div", "spll", base + CCM_CSCR, 26, 3);
	clks[nfc_div] = imx_clk_divider("nfc_div", "ipg", base + CCM_PCDR0, 12, 4);
	clks[lcdc_per_gate] = imx_clk_gate("lcdc_per_gate", "per3", base + CCM_PCCR0, 18);
	clks[lcdc_ahb_gate] = imx_clk_gate("lcdc_ahb_gate", "ahb", base + CCM_PCCR0, 26);
	/*
	 * i.MX21 doesn't have an IPG clock for the LCD. To avoid even more conditionals
	 * in the framebuffer code, provide a dummy clock.
	 */
	clks[lcdc_ipg_gate] = clk_fixed("dummy", 0);

	clkdev_add_physbase(clks[per1], MX21_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_GPT2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_GPT3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX21_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX21_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX21_CSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_I2C_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_SDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_SDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[lcdc_per_gate], MX21_LCDC_BASE_ADDR, "per");
	clkdev_add_physbase(clks[lcdc_ahb_gate], MX21_LCDC_BASE_ADDR, "ahb");
	clkdev_add_physbase(clks[lcdc_ipg_gate], MX21_LCDC_BASE_ADDR, "ipg");

	return 0;
}

static __maybe_unused struct of_device_id imx21_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx21-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx21_ccm_driver = {
	.probe	= imx21_ccm_probe,
	.name	= "imx21-ccm",
	.of_compatible = DRV_OF_COMPAT(imx21_ccm_dt_ids),
};

static int imx21_ccm_init(void)
{
	return platform_driver_register(&imx21_ccm_driver);
}
core_initcall(imx21_ccm_init);
