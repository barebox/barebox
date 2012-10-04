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

enum imx21_clks {
	ckil, ckih, fpm, mpll_sel, spll_sel, mpll, spll, fclk, hclk, ipg, per1,
	per2, per3, per4, usb_div, nfc_div, clk_max
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
	void __iomem *base;
	unsigned long lref = 32768;
	unsigned long href = 26000000;

	base = dev_request_mem_region(dev, 0);

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

	clkdev_add_physbase(clks[per1], MX21_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_GPT2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_GPT3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX21_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX21_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX21_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_I2C_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_SDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX21_SDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per3], MX21_CSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per3], MX21_LCDC_BASE_ADDR, NULL);

	return 0;
}

static struct driver_d imx21_ccm_driver = {
	.probe	= imx21_ccm_probe,
	.name	= "imx21-ccm",
};

static int imx21_ccm_init(void)
{
	return platform_driver_register(&imx21_ccm_driver);
}
postcore_initcall(imx21_ccm_init);
