/*
 * Copyright (C) 2012 Sascha Hauer <kernel@pengutronix.de>
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
 * Foundation.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx31-regs.h>

#include "clk.h"

/* Register addresses */
#define CCM_CCMR		0x00
#define CCM_PDR0		0x04
#define CCM_PDR1		0x08
//#define CCM_RCSR		0x0C
#define CCM_MPCTL		0x10
#define CCM_UPCTL		0x14
#define CCM_SRPCTL		0x18
#define CCM_COSR		0x1C
#define CCM_CGR0		0x20
#define CCM_CGR1		0x24
#define CCM_CGR2		0x28
#define CCM_WIMR		0x2C
#define CCM_LDC		0x30
#define CCM_DCVR0		0x34
#define CCM_DCVR1		0x38
#define CCM_DCVR2		0x3C
#define CCM_DCVR3		0x40
#define CCM_LTR0		0x44
#define CCM_LTR1		0x48
#define CCM_LTR2		0x4C
#define CCM_LTR3		0x50
#define CCM_LTBR0		0x54
#define CCM_LTBR1		0x58
#define CCM_PMCR0		0x5C
#define CCM_PMCR1		0x60
#define CCM_PDR2		0x64

enum mx31_clks {
	ckih, ckil, mpll, spll, upll, mcu_main, hsp, ahb, nfc, ipg, per_div,
	per, csi, fir, csi_div, usb_div_pre, usb_div_post, fir_div_pre,
	fir_div_post, sdhc1_gate, sdhc2_gate, gpt_gate, epit1_gate, epit2_gate,
	iim_gate, ata_gate, sdma_gate, cspi3_gate, rng_gate, uart1_gate,
	uart2_gate, ssi1_gate, i2c1_gate, i2c2_gate, i2c3_gate, hantro_gate,
	mstick1_gate, mstick2_gate, csi_gate, rtc_gate, wdog_gate, pwm_gate,
	sim_gate, ect_gate, usb_gate, kpp_gate, ipu_gate, uart3_gate,
	uart4_gate, uart5_gate, owire_gate, ssi2_gate, cspi1_gate, cspi2_gate,
	gacc_gate, emi_gate, rtic_gate, firi_gate, fpm, pll_ref, mpll_byp,
	clk_max
};

static struct clk *clks[clk_max];

static const char *pll_ref_sel[] = {
	"dummy",
	"fpm",
	"ckih",
	"dummy",
};

static const char *mpll_byp_sel[] = {
	"mpll",
	"pll_ref",
};

static const char *mcu_main_sel[] = {
	"spll",
	"mpll_byp",
};

static const char *per_sel[] = {
	"per_div",
	"ipg",
};

static int imx31_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	writel(0xffffffff, base + CCM_CGR0);
	writel(0xffffffff, base + CCM_CGR1);
	writel(0xffffffff, base + CCM_CGR2);

	clks[ckih] = clk_fixed("ckih", 26000000);
	clks[ckil] = clk_fixed("ckil", 32768);
	clks[fpm] = imx_clk_fixed_factor("fpm", "ckil", 1024, 1);
	clks[pll_ref] = imx_clk_mux("pll_ref", base + CCM_CCMR, 1, 2,
		pll_ref_sel,  ARRAY_SIZE(pll_ref_sel));
	clks[mpll] = imx_clk_pllv1("mpll", "pll_ref", base + CCM_MPCTL);
	clks[spll] = imx_clk_pllv1("spll", "pll_ref", base + CCM_SRPCTL);
	clks[upll] = imx_clk_pllv1("upll", "pll_ref", base + CCM_UPCTL);
	clks[mpll_byp] = imx_clk_mux("mpll_byp", base + CCM_CCMR, 7, 1,
			mpll_byp_sel, ARRAY_SIZE(mpll_byp_sel));
	clks[mcu_main] = imx_clk_mux("mcu_main", base + CCM_PMCR0, 31, 1,
			mcu_main_sel, ARRAY_SIZE(mcu_main_sel));
	clks[hsp] = imx_clk_divider("hsp", "mcu_main", base + CCM_PDR0, 11, 3);
	clks[ahb] = imx_clk_divider("ahb", "mcu_main", base + CCM_PDR0, 3, 3);
	clks[nfc] = imx_clk_divider("nfc", "ahb", base + CCM_PDR0, 8, 3);
	clks[ipg] = imx_clk_divider("ipg", "ahb", base + CCM_PDR0, 6, 2);
	clks[per_div] = imx_clk_divider("per_div", "upll", base + CCM_PDR0, 16, 5);
	clks[per] = imx_clk_mux("per", base + CCM_CCMR, 24, 1, per_sel, ARRAY_SIZE(per_sel));

	clkdev_add_physbase(clks[per], MX31_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_UART5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_I2C3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX31_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX31_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX31_CSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_SDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_SDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per], MX31_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[hsp], MX31_IPU_CTRL_BASE_ADDR, NULL);

	return 0;
}

static __maybe_unused struct of_device_id imx31_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx31-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx31_ccm_driver = {
	.probe	= imx31_ccm_probe,
	.name	= "imx31-ccm",
	.of_compatible = DRV_OF_COMPAT(imx31_ccm_dt_ids),
};

static int imx31_ccm_init(void)
{
	return platform_driver_register(&imx31_ccm_driver);
}
core_initcall(imx31_ccm_init);
