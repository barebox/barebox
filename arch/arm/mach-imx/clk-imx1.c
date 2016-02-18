/*
 *  Copyright (C) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx1-regs.h>

#include "clk.h"

#define CCM_CSCR	0x0
#define CCM_MPCTL0	0x4
#define CCM_SPCTL0	0xc
#define CCM_PCDR	0x20

enum imx1_clks {
	dummy, clk32, clk16m, clk32_premult, prem, mpll, spll, mcu,
	fclk, hclk, clk48m, per1, per2, per3, clko, dma_gate, csi_gate,
	mma_gate, usbd_gate, clk_max
};

static struct clk *clks[clk_max];

static const char *prem_sel_clks[] = {
	"clk32_premult",
	"clk16m",
};

static const char *clko_sel_clks[] = {
	"per1",
	"hclk",
	"clk48m",
	"clk16m",
	"prem",
	"fclk",
};

int __init mx1_clocks_init(void __iomem *regs, unsigned long fref)
{
	clks[dummy] = clk_fixed("dummy", 0);
	clks[clk32] = clk_fixed("clk32", fref);
	clks[clk16m] = clk_fixed("clk16m", 16000000);
	clks[clk32_premult] = imx_clk_fixed_factor("clk32_premult", "clk32", 512, 1);
	clks[prem] = imx_clk_mux("prem", regs + CCM_CSCR, 16, 1, prem_sel_clks,
			ARRAY_SIZE(prem_sel_clks));
	clks[mpll] = imx_clk_pllv1("mpll", "clk32_premult", regs + CCM_MPCTL0);
	clks[spll] = imx_clk_pllv1("spll", "prem", regs + CCM_SPCTL0);
	clks[mcu] = imx_clk_divider("mcu", "clk32_premult", regs + CCM_CSCR, 15, 1);
	clks[fclk] = imx_clk_divider("fclk", "mpll", regs + CCM_CSCR, 15, 1);
	clks[hclk] = imx_clk_divider("hclk", "spll", regs + CCM_CSCR, 10, 4);
	clks[clk48m] = imx_clk_divider("clk48m", "spll", regs + CCM_CSCR, 26, 3);
	clks[per1] = imx_clk_divider("per1", "spll", regs + CCM_PCDR, 0, 4);
	clks[per2] = imx_clk_divider("per2", "spll", regs + CCM_PCDR, 4, 4);
	clks[per3] = imx_clk_divider("per3", "spll", regs + CCM_PCDR, 16, 7);
	clks[clko] = imx_clk_mux("clko", regs + CCM_CSCR, 29, 3, clko_sel_clks,
			ARRAY_SIZE(clko_sel_clks));

	clkdev_add_physbase(clks[per1], MX1_TIM1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX1_TIM2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX1_LCDC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX1_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1], MX1_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX1_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2], MX1_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[hclk], MX1_I2C_BASE_ADDR, NULL);

	return 0;
}

static int imx1_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx1_clocks_init(regs, 32000);

	return 0;
}

static __maybe_unused struct of_device_id imx1_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx1-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx1_ccm_driver = {
	.probe	= imx1_ccm_probe,
	.name	= "imx1-ccm",
	.of_compatible = DRV_OF_COMPAT(imx1_ccm_dt_ids),
};

static int imx1_ccm_init(void)
{
	return platform_driver_register(&imx1_ccm_driver);
}
core_initcall(imx1_ccm_init);
