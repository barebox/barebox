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
#include <mach/imx28-regs.h>

#include "clk.h"

#define PLL0CTRL0		(regs + 0x0000)
#define PLL1CTRL0		(regs + 0x0020)
#define PLL2CTRL0		(regs + 0x0040)
#define CPU			(regs + 0x0050)
#define HBUS			(regs + 0x0060)
#define XBUS			(regs + 0x0070)
#define XTAL			(regs + 0x0080)
#define SSP0			(regs + 0x0090)
#define SSP1			(regs + 0x00a0)
#define SSP2			(regs + 0x00b0)
#define SSP3			(regs + 0x00c0)
#define GPMI			(regs + 0x00d0)
#define SPDIF			(regs + 0x00e0)
#define EMI			(regs + 0x00f0)
#define SAIF0			(regs + 0x0100)
#define SAIF1			(regs + 0x0110)
#define LCDIF			(regs + 0x0120)
#define ETM			(regs + 0x0130)
#define ENET			(regs + 0x0140)
#define FLEXCAN			(regs + 0x0160)
#define FRAC0			(regs + 0x01b0)
#define FRAC1			(regs + 0x01c0)
#define CLKSEQ			(regs + 0x01d0)

static const char *sel_cpu[]  = { "ref_cpu", "ref_xtal", };
static const char *sel_io0[]  = { "ref_io0", "ref_xtal", };
static const char *sel_io1[]  = { "ref_io1", "ref_xtal", };
static const char *sel_pix[]  = { "ref_pix", "ref_xtal", };
static const char *sel_gpmi[] = { "ref_gpmi", "ref_xtal", };
static const char *cpu_sels[] = { "cpu_pll", "cpu_xtal", };
static const char *emi_sels[] = { "emi_pll", "emi_xtal", };
static const char *ptp_sels[] = { "ref_xtal", "pll0", };

enum imx28_clk {
	ref_xtal, pll0, pll1, pll2, ref_cpu, ref_emi, ref_io0, ref_io1,
	ref_pix, ref_hsadc, ref_gpmi, saif0_sel, saif1_sel, gpmi_sel,
	ssp0_sel, ssp1_sel, ssp2_sel, ssp3_sel, emi_sel, etm_sel,
	lcdif_sel, cpu, ptp_sel, cpu_pll, cpu_xtal, hbus, xbus,
	ssp0_gate, ssp1_gate, ssp2_gate, ssp3_gate, gpmi_div, emi_pll,
	emi_xtal, lcdif_div, etm_div, ptp, saif0_div, saif1_div,
	clk32k_div, rtc, lradc, spdif_div, clk32k, pwm, uart, ssp0,
	ssp1, ssp2, ssp3, gpmi, spdif, emi, saif0, saif1, lcdif, etm,
	fec_sleep, fec, can0, can1, usb0, usb1, usb0_phy, usb1_phy, enet_out,
	lcdif_comp, clk_max
};

static struct clk *clks[clk_max];

int __init mx28_clocks_init(void __iomem *regs)
{
	clks[ref_xtal] = clk_fixed("ref_xtal", 24000000);
	clks[pll0] = mxs_clk_pll("pll0", "ref_xtal", PLL0CTRL0, 17, 480000000);
	clks[pll1] = mxs_clk_pll("pll1", "ref_xtal", PLL1CTRL0, 17, 480000000);
	clks[pll2] = mxs_clk_pll("pll2", "ref_xtal", PLL2CTRL0, 23, 50000000);
	clks[ref_cpu] = mxs_clk_ref("ref_cpu", "pll0", FRAC0, 0);
	clks[ref_emi] = mxs_clk_ref("ref_emi", "pll0", FRAC0, 1);
	clks[ref_io1] = mxs_clk_ref("ref_io1", "pll0", FRAC0, 2);
	clks[ref_io0] = mxs_clk_ref("ref_io0", "pll0", FRAC0, 3);
	clks[ref_pix] = mxs_clk_ref("ref_pix", "pll0", FRAC1, 0);
	clks[ref_hsadc] = mxs_clk_ref("ref_hsadc", "pll0", FRAC1, 1);
	clks[ref_gpmi] = mxs_clk_ref("ref_gpmi", "pll0", FRAC1, 2);
	clks[gpmi_sel] = mxs_clk_mux("gpmi_sel", CLKSEQ, 2, 1, sel_gpmi, ARRAY_SIZE(sel_gpmi));
	clks[ssp0_sel] = mxs_clk_mux("ssp0_sel", CLKSEQ, 3, 1, sel_io0, ARRAY_SIZE(sel_io0));
	clks[ssp1_sel] = mxs_clk_mux("ssp1_sel", CLKSEQ, 4, 1, sel_io0, ARRAY_SIZE(sel_io0));
	clks[ssp2_sel] = mxs_clk_mux("ssp2_sel", CLKSEQ, 5, 1, sel_io1, ARRAY_SIZE(sel_io1));
	clks[ssp3_sel] = mxs_clk_mux("ssp3_sel", CLKSEQ, 6, 1, sel_io1, ARRAY_SIZE(sel_io1));
	clks[emi_sel] = mxs_clk_mux("emi_sel", CLKSEQ, 7, 1, emi_sels, ARRAY_SIZE(emi_sels));
	clks[etm_sel] = mxs_clk_mux("etm_sel", CLKSEQ, 8, 1, sel_cpu, ARRAY_SIZE(sel_cpu));
	clks[lcdif_sel] = mxs_clk_mux("lcdif_sel", CLKSEQ, 14, 1, sel_pix, ARRAY_SIZE(sel_pix));
	clks[cpu] = mxs_clk_mux("cpu", CLKSEQ, 18, 1, cpu_sels, ARRAY_SIZE(cpu_sels));
	clks[ptp_sel] = mxs_clk_mux("ptp_sel", ENET, 19, 1, ptp_sels, ARRAY_SIZE(ptp_sels));
	clks[cpu_pll] = mxs_clk_div("cpu_pll", "ref_cpu", CPU, 0, 6, 28);
	clks[cpu_xtal] = mxs_clk_div("cpu_xtal", "ref_xtal", CPU, 16, 10, 29);
	clks[hbus] = mxs_clk_div("hbus", "cpu", HBUS, 0, 5, 31);
	clks[xbus] = mxs_clk_div("xbus", "ref_xtal", XBUS, 0, 10, 31);
	clks[ssp0_gate] = mxs_clk_gate("ssp0_gate", "ssp0_sel", SSP0, 31);
	clks[ssp1_gate] = mxs_clk_gate("ssp1_gate", "ssp1_sel", SSP1, 31);
	clks[ssp2_gate] = mxs_clk_gate("ssp2_gate", "ssp2_sel", SSP2, 31);
	clks[ssp3_gate] = mxs_clk_gate("ssp3_gate", "ssp3_sel", SSP3, 31);
	clks[ssp0] = mxs_clk_div("ssp0", "ssp0_gate", SSP0, 0, 9, 29);
	clks[ssp1] = mxs_clk_div("ssp1", "ssp1_gate", SSP1, 0, 9, 29);
	clks[ssp2] = mxs_clk_div("ssp2", "ssp2_gate", SSP2, 0, 9, 29);
	clks[ssp3] = mxs_clk_div("ssp3", "ssp3_gate", SSP3, 0, 9, 29);
	clks[gpmi_div] = mxs_clk_div("gpmi_div", "gpmi_sel", GPMI, 0, 10, 29);
	clks[emi_pll] = mxs_clk_div("emi_pll", "ref_emi", EMI, 0, 6, 28);
	clks[emi_xtal] = mxs_clk_div("emi_xtal", "ref_xtal", EMI, 8, 4, 29);
	clks[lcdif_div] = mxs_clk_div("lcdif_div", "lcdif_sel", LCDIF, 0, 13, 29);
	clks[etm_div] = mxs_clk_div("etm_div", "etm_sel", ETM, 0, 7, 29);
	clks[clk32k_div] = mxs_clk_fixed_factor("clk32k_div", "ref_xtal", 1, 750);
	clks[rtc] = mxs_clk_fixed_factor("rtc", "ref_xtal", 1, 768);
	clks[lradc] = mxs_clk_fixed_factor("lradc", "clk32k", 1, 16);
	clks[clk32k] = mxs_clk_gate("clk32k", "clk32k_div", XTAL, 26);
	clks[pwm] = mxs_clk_gate("pwm", "ref_xtal", XTAL, 29);
	clks[uart] = mxs_clk_gate("uart", "ref_xtal", XTAL, 31);
	clks[gpmi] = mxs_clk_gate("gpmi", "gpmi_div", GPMI, 31);
	clks[emi] = mxs_clk_gate("emi", "emi_sel", EMI, 31);
	clks[lcdif] = mxs_clk_gate("lcdif", "lcdif_div", LCDIF, 31);
	clks[etm] = mxs_clk_gate("etm", "etm_div", ETM, 31);
	clks[fec_sleep] = mxs_clk_gate("fec_sleep", "hbus", ENET, 31);
	clks[fec] = mxs_clk_gate("fec", "fec_sleep", ENET, 30);
	clks[usb0_phy] = mxs_clk_gate("usb0_phy", "pll0", PLL0CTRL0, 18);
	clks[usb1_phy] = mxs_clk_gate("usb1_phy", "pll1", PLL1CTRL0, 18);
	clks[enet_out] = clk_gate("enet_out", "pll2", ENET, 18, 0, 0);
	clks[lcdif_comp] = mxs_clk_lcdif("lcdif_comp", clks[ref_pix],
			clks[lcdif_div], clks[lcdif]);

	clk_set_rate(clks[ref_io0], 480000000);
	clk_set_rate(clks[ref_io1], 480000000);
	clk_set_parent(clks[ssp0_sel], clks[ref_io0]);
	clk_set_parent(clks[ssp1_sel], clks[ref_io0]);
	clk_set_parent(clks[ssp2_sel], clks[ref_io1]);
	clk_set_parent(clks[ssp3_sel], clks[ref_io1]);
	clk_set_rate(clks[ssp0], 96000000);
	clk_set_rate(clks[ssp1], 96000000);
	clk_set_rate(clks[ssp2], 96000000);
	clk_set_rate(clks[ssp3], 96000000);
	clk_set_parent(clks[lcdif_sel], clks[ref_pix]);
	clk_enable(clks[enet_out]);

	clkdev_add_physbase(clks[ssp0], IMX_SSP0_BASE, NULL);
	clkdev_add_physbase(clks[ssp1], IMX_SSP1_BASE, NULL);
	clkdev_add_physbase(clks[ssp2], IMX_SSP2_BASE, NULL);
	clkdev_add_physbase(clks[ssp3], IMX_SSP3_BASE, NULL);
	clkdev_add_physbase(clks[fec], IMX_FEC0_BASE, NULL);
	clkdev_add_physbase(clks[xbus], IMX_DBGUART_BASE, NULL);
	clkdev_add_physbase(clks[hbus], IMX_OCOTP_BASE, NULL);
	clkdev_add_physbase(clks[hbus], MXS_APBH_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART0_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART1_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART2_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART3_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART4_BASE, NULL);
	clkdev_add_physbase(clks[gpmi], MXS_GPMI_BASE, NULL);
	clkdev_add_physbase(clks[pwm], IMX_PWM_BASE, NULL);
	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_STM))
		clkdev_add_physbase(clks[lcdif_comp], IMX_FB_BASE, NULL);

	return 0;
}

static int imx28_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx28_clocks_init(regs);

	return 0;
}

static __maybe_unused struct of_device_id imx28_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx28-clkctrl",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx28_ccm_driver = {
	.probe	= imx28_ccm_probe,
	.name	= "imx28-clkctrl",
	.of_compatible = DRV_OF_COMPAT(imx28_ccm_dt_ids),
};

static int imx28_ccm_init(void)
{
	return platform_driver_register(&imx28_ccm_driver);
}
postcore_initcall(imx28_ccm_init);
