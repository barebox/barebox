/*
 * Copyright (C) 2009 by Sascha Hauer, Pengutronix
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
#include <mach/imx25-regs.h>

#include "clk.h"

#define CCM_MPCTL	0x00
#define CCM_UPCTL	0x04
#define CCM_CCTL	0x08
#define CCM_CGCR0	0x0C
#define CCM_CGCR1	0x10
#define CCM_CGCR2	0x14
#define CCM_PCDR0	0x18
#define CCM_PCDR1	0x1C
#define CCM_PCDR2	0x20
#define CCM_PCDR3	0x24
#define CCM_RCSR	0x28
#define CCM_CRDR	0x2C
#define CCM_DCVR0	0x30
#define CCM_DCVR1	0x34
#define CCM_DCVR2	0x38
#define CCM_DCVR3	0x3c
#define CCM_LTR0	0x40
#define CCM_LTR1	0x44
#define CCM_LTR2	0x48
#define CCM_LTR3	0x4c
#define CCM_MCR		0x64

enum mx25_clks {
	dummy, osc, mpll, upll, mpll_cpu_3_4, cpu_sel, cpu, ahb, usb_div, ipg,
	per0_sel, per1_sel, per2_sel, per3_sel, per4_sel, per5_sel, per6_sel,
	per7_sel, per8_sel, per9_sel, per10_sel, per11_sel, per12_sel,
	per13_sel, per14_sel, per15_sel, per0, per1, per2, per3, per4, per5,
	per6, per7, per8, per9, per10, per11, per12, per13, per14, per15,
	csi_ipg_per, epit_ipg_per, esai_ipg_per, esdhc1_ipg_per, esdhc2_ipg_per,
	gpt_ipg_per, i2c_ipg_per, lcdc_ipg_per, nfc_ipg_per, owire_ipg_per,
	pwm_ipg_per, sim1_ipg_per, sim2_ipg_per, ssi1_ipg_per, ssi2_ipg_per,
	uart_ipg_per, ata_ahb, reserved1, csi_ahb, emi_ahb, esai_ahb, esdhc1_ahb,
	esdhc2_ahb, fec_ahb, lcdc_ahb, rtic_ahb, sdma_ahb, slcdc_ahb, usbotg_ahb,
	reserved2, reserved3, reserved4, reserved5, can1_ipg, can2_ipg, csi_ipg,
	cspi1_ipg, cspi2_ipg, cspi3_ipg, dryice_ipg, ect_ipg, epit1_ipg, epit2_ipg,
	reserved6, esdhc1_ipg, esdhc2_ipg, fec_ipg, reserved7, reserved8, reserved9,
	gpt1_ipg, gpt2_ipg, gpt3_ipg, gpt4_ipg, reserved10, reserved11, reserved12,
	iim_ipg, reserved13, reserved14, kpp_ipg, lcdc_ipg, reserved15, pwm1_ipg,
	pwm2_ipg, pwm3_ipg, pwm4_ipg, rngb_ipg, reserved16, scc_ipg, sdma_ipg,
	sim1_ipg, sim2_ipg, slcdc_ipg, spba_ipg, ssi1_ipg, ssi2_ipg, tsc_ipg,
	uart1_ipg, uart2_ipg, uart3_ipg, uart4_ipg, uart5_ipg, reserved17,
	wdt_ipg, clk_max
};

static struct clk *clks[clk_max];

static const char *cpu_sel_clks[] = {
	"mpll",
	"mpll_cpu_3_4",
};

static const char *per_sel_clks[] = {
	"ahb",
	"upll",
};

static int imx25_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	writel((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 9) |
			(1 << 10) | (1 << 15) |	(1 << 19) | (1 << 21) | (1 << 22) |
			(1 << 23) | (1 << 24) | (1 << 28),
			base + CCM_CGCR0);

	writel((1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 13) | (1 << 14) |
			(1 << 15) | (1 << 19) | (1 << 20) | (1 << 21) | (1 << 22) |
			(1 << 26) | (1 << 31),
			base + CCM_CGCR1);

	writel((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 10) |
	       (1 << 13) | (1 << 14) | (1 << 15) | (1 << 16) | (1 << 17) | (1 << 18),
			base + CCM_CGCR2);

	clks[dummy] = clk_fixed("dummy", 0);
	clks[osc] = clk_fixed("osc", 24000000);
	clks[mpll] = imx_clk_pllv1("mpll", "osc", base + CCM_MPCTL);
	clks[upll] = imx_clk_pllv1("upll", "osc", base + CCM_UPCTL);
	clks[mpll_cpu_3_4] = imx_clk_fixed_factor("mpll_cpu_3_4", "mpll", 3, 4);
	clks[cpu_sel] = imx_clk_mux("cpu_sel", base + CCM_CCTL, 14, 1, cpu_sel_clks, ARRAY_SIZE(cpu_sel_clks));
	clks[cpu] = imx_clk_divider("cpu", "cpu_sel", base + CCM_CCTL, 30, 2);
	clks[ahb] = imx_clk_divider("ahb", "cpu", base + CCM_CCTL, 28, 2);
	clks[usb_div] = imx_clk_divider("usb_div", "upll", base + CCM_CCTL, 16, 6);
	clks[ipg] = imx_clk_fixed_factor("ipg", "ahb", 1, 2);
	clks[per0_sel] = imx_clk_mux("per0_sel", base + CCM_MCR, 0, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per1_sel] = imx_clk_mux("per1_sel", base + CCM_MCR, 1, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per2_sel] = imx_clk_mux("per2_sel", base + CCM_MCR, 2, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per3_sel] = imx_clk_mux("per3_sel", base + CCM_MCR, 3, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per4_sel] = imx_clk_mux("per4_sel", base + CCM_MCR, 4, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per5_sel] = imx_clk_mux("per5_sel", base + CCM_MCR, 5, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per6_sel] = imx_clk_mux("per6_sel", base + CCM_MCR, 6, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per7_sel] = imx_clk_mux("per7_sel", base + CCM_MCR, 7, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per8_sel] = imx_clk_mux("per8_sel", base + CCM_MCR, 8, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per9_sel] = imx_clk_mux("per9_sel", base + CCM_MCR, 9, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per10_sel] = imx_clk_mux("per10_sel", base + CCM_MCR, 10, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per11_sel] = imx_clk_mux("per11_sel", base + CCM_MCR, 11, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per12_sel] = imx_clk_mux("per12_sel", base + CCM_MCR, 12, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per13_sel] = imx_clk_mux("per13_sel", base + CCM_MCR, 13, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per14_sel] = imx_clk_mux("per14_sel", base + CCM_MCR, 14, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per15_sel] = imx_clk_mux("per15_sel", base + CCM_MCR, 15, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clks[per0] = imx_clk_divider("per0", "per0_sel", base + CCM_PCDR0, 0, 6);
	clks[per1] = imx_clk_divider("per1", "per1_sel", base + CCM_PCDR0, 8, 6);
	clks[per2] = imx_clk_divider("per2", "per2_sel", base + CCM_PCDR0, 16, 6);
	clks[per3] = imx_clk_divider("per3", "per3_sel", base + CCM_PCDR0, 24, 6);
	clks[per4] = imx_clk_divider("per4", "per4_sel", base + CCM_PCDR1, 0, 6);
	clks[per5] = imx_clk_divider("per5", "per5_sel", base + CCM_PCDR1, 8, 6);
	clks[per6] = imx_clk_divider("per6", "per6_sel", base + CCM_PCDR1, 16, 6);
	clks[per7] = imx_clk_divider("per7", "per7_sel", base + CCM_PCDR1, 24, 6);
	clks[per8] = imx_clk_divider("per8", "per8_sel", base + CCM_PCDR2, 0, 6);
	clks[per9] = imx_clk_divider("per9", "per9_sel", base + CCM_PCDR2, 8, 6);
	clks[per10] = imx_clk_divider("per10", "per10_sel", base + CCM_PCDR2, 16, 6);
	clks[per11] = imx_clk_divider("per11", "per11_sel", base + CCM_PCDR2, 24, 6);
	clks[per12] = imx_clk_divider("per12", "per12_sel", base + CCM_PCDR3, 0, 6);
	clks[per13] = imx_clk_divider("per13", "per13_sel", base + CCM_PCDR3, 8, 6);
	clks[per14] = imx_clk_divider("per14", "per14_sel", base + CCM_PCDR3, 16, 6);
	clks[per15] = imx_clk_divider("per15", "per15_sel", base + CCM_PCDR3, 24, 6);
	clks[lcdc_ahb] = imx_clk_gate("lcdc_ahb", "ahb", base + CCM_CGCR0, 24);
	clks[lcdc_ipg] = imx_clk_gate("lcdc_ipg", "ipg", base + CCM_CGCR1, 29);
	clks[lcdc_ipg_per] = imx_clk_gate("lcdc_ipg_per", "per7", base + CCM_CGCR0, 7);
	clks[rngb_ipg] = imx_clk_gate("rngb_ipg", "ipg", base + CCM_CGCR2, 3);

	clkdev_add_physbase(clks[per15], MX25_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per15], MX25_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per15], MX25_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per15], MX25_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per15], MX25_UART5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per5], MX25_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per5], MX25_GPT2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per5], MX25_GPT3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per5], MX25_GPT4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_FEC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_I2C3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX25_CSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per3], MX25_ESDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per4], MX25_ESDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per8], MX25_NFC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[lcdc_ipg_per], MX25_LCDC_BASE_ADDR, "per");
	clkdev_add_physbase(clks[lcdc_ipg], MX25_LCDC_BASE_ADDR, "ipg");
	clkdev_add_physbase(clks[lcdc_ahb], MX25_LCDC_BASE_ADDR, "ahb");

	return 0;
}

static __maybe_unused struct of_device_id imx25_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx25-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx25_ccm_driver = {
	.probe	= imx25_ccm_probe,
	.name	= "imx25-ccm",
	.of_compatible = DRV_OF_COMPAT(imx25_ccm_dt_ids),
};

static int imx25_ccm_init(void)
{
	return platform_driver_register(&imx25_ccm_driver);
}
core_initcall(imx25_ccm_init);
