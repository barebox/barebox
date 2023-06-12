// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/mxs/imx23-regs.h>

#include "clk.h"

#define PLLCTRL0		(regs + 0x0000)
#define CPU			(regs + 0x0020)
#define HBUS			(regs + 0x0030)
#define XBUS			(regs + 0x0040)
#define XTAL			(regs + 0x0050)
#define PIX			(regs + 0x0060)
#define SSP			(regs + 0x0070)
#define GPMI			(regs + 0x0080)
#define SPDIF			(regs + 0x0090)
#define EMI			(regs + 0x00a0)
#define SAIF			(regs + 0x00c0)
#define TV			(regs + 0x00d0)
#define ETM			(regs + 0x00e0)
#define FRAC			(regs + 0x00f0)
#define CLKSEQ			(regs + 0x0110)

static const char *sel_pll[]  = { "pll", "ref_xtal", };
static const char *sel_cpu[]  = { "ref_cpu", "ref_xtal", };
static const char *sel_pix[]  = { "ref_pix", "ref_xtal", };
static const char *sel_io[]   = { "ref_io", "ref_xtal", };
static const char *cpu_sels[] = { "cpu_pll", "cpu_xtal", };
static const char *emi_sels[] = { "emi_pll", "emi_xtal", };

enum imx23_clk {
	ref_xtal, pll, ref_cpu, ref_emi, ref_pix, ref_io, saif_sel,
	lcdif_sel, gpmi_sel, ssp_sel, emi_sel, cpu, etm_sel, cpu_pll,
	cpu_xtal, hbus, xbus, lcdif_div, ssp_div, gpmi_div, emi_pll,
	emi_xtal, etm_div, saif_div, clk32k_div, rtc, adc, spdif_div,
	clk32k, dri, pwm, filt, uart, ssp, gpmi, spdif, emi, saif,
	lcdif, etm, usb, usb_phy, lcdif_comp,
	clk_max
};

static struct clk *clks[clk_max];

static int __init mx23_clocks_init(void __iomem *regs)
{
	clks[ref_xtal] = mxs_clk_fixed("ref_xtal", 24000000);
	clks[pll] = mxs_clk_pll("pll", "ref_xtal", PLLCTRL0, 16, 480000000);
	clks[ref_cpu] = mxs_clk_ref("ref_cpu", "pll", FRAC, 0);
	clks[ref_emi] = mxs_clk_ref("ref_emi", "pll", FRAC, 1);
	clks[ref_pix] = mxs_clk_ref("ref_pix", "pll", FRAC, 2);
	clks[ref_io] = mxs_clk_ref("ref_io", "pll", FRAC, 3);
	clks[saif_sel] = mxs_clk_mux("saif_sel", CLKSEQ, 0, 1, sel_pll, ARRAY_SIZE(sel_pll));
	clks[lcdif_sel] = mxs_clk_mux("lcdif_sel", CLKSEQ, 1, 1, sel_pix, ARRAY_SIZE(sel_pix));
	clks[gpmi_sel] = mxs_clk_mux("gpmi_sel", CLKSEQ, 4, 1, sel_io, ARRAY_SIZE(sel_io));
	clks[ssp_sel] = mxs_clk_mux("ssp_sel", CLKSEQ, 5, 1, sel_io, ARRAY_SIZE(sel_io));
	clks[emi_sel] = mxs_clk_mux("emi_sel", CLKSEQ, 6, 1, emi_sels, ARRAY_SIZE(emi_sels));
	clks[cpu] = mxs_clk_mux("cpu", CLKSEQ, 7, 1, cpu_sels, ARRAY_SIZE(cpu_sels));
	clks[etm_sel] = mxs_clk_mux("etm_sel", CLKSEQ, 8, 1, sel_cpu, ARRAY_SIZE(sel_cpu));
	clks[cpu_pll] = mxs_clk_div("cpu_pll", "ref_cpu", CPU, 0, 6, 28);
	clks[cpu_xtal] = mxs_clk_div("cpu_xtal", "ref_xtal", CPU, 16, 10, 29);
	clks[hbus] = mxs_clk_div("hbus", "cpu", HBUS, 0, 5, 29);
	clks[xbus] = mxs_clk_div("xbus", "ref_xtal", XBUS, 0, 10, 31);
	clks[lcdif_div] = mxs_clk_div("lcdif_div", "lcdif_sel", PIX, 0, 12, 29);
	clks[ssp_div] = mxs_clk_div("ssp_div", "ssp_sel", SSP, 0, 9, 29);
	clks[gpmi_div] = mxs_clk_div("gpmi_div", "gpmi_sel", GPMI, 0, 10, 29);
	clks[emi_pll] = mxs_clk_div("emi_pll", "ref_emi", EMI, 0, 6, 28);
	clks[emi_xtal] = mxs_clk_div("emi_xtal", "ref_xtal", EMI, 8, 4, 29);
	clks[etm_div] = mxs_clk_div("etm_div", "etm_sel", ETM, 0, 6, 29);
	clks[saif_div] = mxs_clk_frac("saif_div", "saif_sel", SAIF, 0, 16, 29);
	clks[clk32k_div] = mxs_clk_fixed_factor("clk32k_div", "ref_xtal", 1, 750);
	clks[rtc] = mxs_clk_fixed_factor("rtc", "ref_xtal", 1, 768);
	clks[adc] = mxs_clk_fixed_factor("adc", "clk32k", 1, 16);
	clks[spdif_div] = mxs_clk_fixed_factor("spdif_div", "pll", 1, 4);
	clks[clk32k] = mxs_clk_gate("clk32k", "clk32k_div", XTAL, 26);
	clks[dri] = mxs_clk_gate("dri", "ref_xtal", XTAL, 28);
	clks[pwm] = mxs_clk_gate("pwm", "ref_xtal", XTAL, 29);
	clks[filt] = mxs_clk_gate("filt", "ref_xtal", XTAL, 30);
	clks[uart] = mxs_clk_gate("uart", "ref_xtal", XTAL, 31);
	clks[ssp] = mxs_clk_gate("ssp", "ssp_div", SSP, 31);
	clks[gpmi] = mxs_clk_gate("gpmi", "gpmi_div", GPMI, 31);
	clks[spdif] = mxs_clk_gate("spdif", "spdif_div", SPDIF, 31);
	clks[emi] = mxs_clk_gate("emi", "emi_sel", EMI, 31);
	clks[saif] = mxs_clk_gate("saif", "saif_div", SAIF, 31);
	clks[lcdif] = mxs_clk_gate("lcdif", "lcdif_div", PIX, 31);
	clks[etm] = mxs_clk_gate("etm", "etm_div", ETM, 31);
	clks[lcdif_comp] = mxs_clk_lcdif("lcdif_comp", clks[ref_pix],
			clks[lcdif_div], clks[lcdif]);

	clk_set_rate(clks[ref_io], 480000000);
	clk_set_parent(clks[ssp_sel], clks[ref_io]);
	clk_set_rate(clks[ssp_div], 96000000);
	clk_set_parent(clks[lcdif_sel], clks[ref_pix]);

	clkdev_add_physbase(clks[ssp], IMX_SSP1_BASE, NULL);
	clkdev_add_physbase(clks[ssp], IMX_SSP2_BASE, NULL);
	clkdev_add_physbase(clks[xbus], IMX_DBGUART_BASE, NULL);
	clkdev_add_physbase(clks[hbus], IMX_OCOTP_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART1_BASE, NULL);
	clkdev_add_physbase(clks[uart], IMX_UART2_BASE, NULL);
	clkdev_add_physbase(clks[gpmi], MXS_GPMI_BASE, NULL);
	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_STM))
		clkdev_add_physbase(clks[lcdif_comp], IMX_FB_BASE, NULL);

	return 0;
}

static int imx23_ccm_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx23_clocks_init(regs);

	return 0;
}

static __maybe_unused struct of_device_id imx23_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx23-clkctrl",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, imx23_ccm_dt_ids);

static struct driver imx23_ccm_driver = {
	.probe	= imx23_ccm_probe,
	.name	= "imx23-clkctrl",
	.of_compatible = DRV_OF_COMPAT(imx23_ccm_dt_ids),
};

postcore_platform_driver(imx23_ccm_driver);
