/*
 * Copyright 2011 Freescale Semiconductor, Inc.
 * Copyright 2011 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx6-regs.h>

#include "clk.h"

#define CCGR0				0x68
#define CCGR1				0x6c
#define CCGR2				0x70
#define CCGR3				0x74
#define CCGR4				0x78
#define CCGR5				0x7c
#define CCGR6				0x80
#define CCGR7				0x84

#define CLPCR				0x54
#define BP_CLPCR_LPM			0
#define BM_CLPCR_LPM			(0x3 << 0)
#define BM_CLPCR_BYPASS_PMIC_READY	(0x1 << 2)
#define BM_CLPCR_ARM_CLK_DIS_ON_LPM	(0x1 << 5)
#define BM_CLPCR_SBYOS			(0x1 << 6)
#define BM_CLPCR_DIS_REF_OSC		(0x1 << 7)
#define BM_CLPCR_VSTBY			(0x1 << 8)
#define BP_CLPCR_STBY_COUNT		9
#define BM_CLPCR_STBY_COUNT		(0x3 << 9)
#define BM_CLPCR_COSC_PWRDOWN		(0x1 << 11)
#define BM_CLPCR_WB_PER_AT_LPM		(0x1 << 16)
#define BM_CLPCR_WB_CORE_AT_LPM		(0x1 << 17)
#define BM_CLPCR_BYP_MMDC_CH0_LPM_HS	(0x1 << 19)
#define BM_CLPCR_BYP_MMDC_CH1_LPM_HS	(0x1 << 21)
#define BM_CLPCR_MASK_CORE0_WFI		(0x1 << 22)
#define BM_CLPCR_MASK_CORE1_WFI		(0x1 << 23)
#define BM_CLPCR_MASK_CORE2_WFI		(0x1 << 24)
#define BM_CLPCR_MASK_CORE3_WFI		(0x1 << 25)
#define BM_CLPCR_MASK_SCU_IDLE		(0x1 << 26)
#define BM_CLPCR_MASK_L2CC_IDLE		(0x1 << 27)

enum mx6q_clks {
	dummy, ckil, ckih, osc, pll2_pfd0_352m, pll2_pfd1_594m, pll2_pfd2_396m,
	pll3_pfd0_720m, pll3_pfd1_540m, pll3_pfd2_508m, pll3_pfd3_454m,
	pll2_198m, pll3_120m, pll3_80m, pll3_60m, twd, step, pll1_sw,
	periph_pre, periph2_pre, periph_clk2_sel, periph2_clk2_sel, axi_sel,
	esai_sel, asrc_sel, spdif_sel, gpu2d_axi, gpu3d_axi, gpu2d_core_sel,
	gpu3d_core_sel, gpu3d_shader_sel, ipu1_sel, ipu2_sel, ldb_di0_sel,
	ldb_di1_sel, ipu1_di0_pre_sel, ipu1_di1_pre_sel, ipu2_di0_pre_sel,
	ipu2_di1_pre_sel, ipu1_di0_sel, ipu1_di1_sel, ipu2_di0_sel,
	ipu2_di1_sel, hsi_tx_sel, pcie_axi_sel, ssi1_sel, ssi2_sel, ssi3_sel,
	usdhc1_sel, usdhc2_sel, usdhc3_sel, usdhc4_sel, enfc_sel, emi_sel,
	emi_slow_sel, vdo_axi_sel, vpu_axi_sel, cko1_sel, periph, periph2,
	periph_clk2, periph2_clk2, ipg, ipg_per, esai_pred, esai_podf,
	asrc_pred, asrc_podf, spdif_pred, spdif_podf, can_root, ecspi_root,
	gpu2d_core_podf, gpu3d_core_podf, gpu3d_shader, ipu1_podf, ipu2_podf,
	ldb_di0_podf, ldb_di1_podf, ipu1_di0_pre, ipu1_di1_pre, ipu2_di0_pre,
	ipu2_di1_pre, hsi_tx_podf, ssi1_pred, ssi1_podf, ssi2_pred, ssi2_podf,
	ssi3_pred, ssi3_podf, uart_serial_podf, usdhc1_podf, usdhc2_podf,
	usdhc3_podf, usdhc4_podf, enfc_pred, enfc_podf, emi_podf,
	emi_slow_podf, vpu_axi_podf, cko1_podf, axi, mmdc_ch0_axi_podf,
	mmdc_ch1_axi_podf, arm, ahb, apbh_dma, asrc, can1_ipg, can1_serial,
	can2_ipg, can2_serial, ecspi1, ecspi2, ecspi3, ecspi4, ecspi5, enet,
	esai, gpt_ipg, gpt_ipg_per, gpu2d_core, gpu3d_core, hdmi_iahb,
	hdmi_isfr, i2c1, i2c2, i2c3, iim, enfc, ipu1, ipu1_di0, ipu1_di1, ipu2,
	ipu2_di0, ldb_di0, ldb_di1, ipu2_di1, hsi_tx, mlb, mmdc_ch0_axi,
	mmdc_ch1_axi, ocram, openvg_axi, pcie_axi, pwm1, pwm2, pwm3, pwm4, per1_bch,
	gpmi_bch_apb, gpmi_bch, gpmi_io, gpmi_apb, sata, sdma, spba, ssi1,
	ssi2, ssi3, uart_ipg, uart_serial, usboh3, usdhc1, usdhc2, usdhc3,
	usdhc4, vdo_axi, vpu_axi, cko1, pll1_sys, pll2_bus, pll3_usb_otg,
	pll4_audio, pll5_video, pll8_mlb, pll7_usb_host, pll6_enet, ssi1_ipg,
	ssi2_ipg, ssi3_ipg, rom, usbphy1, usbphy2, ldb_di0_div_3_5, ldb_di1_div_3_5,
	sata_ref, pcie_ref, sata_ref_100m, pcie_ref_125m, enet_ref,
	clk_max
};

static struct clk *clks[clk_max];

static const char *step_sels[] = {
	"osc",
	"pll2_pfd2_396m",
};

static const char *pll1_sw_sels[] = {
	"pll1_sys",
	"step",
};

static const char *periph_pre_sels[] = {
	"pll2_bus",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m",
	"pll2_198m",
};

static const char *periph_clk2_sels[] = {
	"pll3_usb_otg",
	"osc",
};

static const char *periph_sels[] = {
	"periph_pre",
	"periph_clk2",
};

static const char *periph2_sels[] = {
	"periph2_pre",
	"periph2_clk2",
};

static const char *axi_sels[] = {
	"periph",
	"pll2_pfd2_396m",
	"pll3_pfd1_540m",
};

static const char *usdhc_sels[] = {
	"pll2_pfd2_396m",
	"pll2_pfd0_352m",
};

static const char *enfc_sels[]	= {
	"pll2_pfd0_352m",
	"pll2_bus",
	"pll3_usb_otg",
	"pll2_pfd2_396m",
};

static const char *emi_sels[] = {
	"axi",
	"pll3_usb_otg",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m",
};

static const char *vdo_axi_sels[] = {
	"axi",
	"ahb",
};

static const char *cko1_sels[] = {
	"pll3_usb_otg",
	"pll2_bus",
	"pll1_sys",
	"pll5_video",
	"dummy",
	"axi",
	"enfc",
	"ipu1_di0",
	"ipu1_di1",
	"ipu2_di0",
	"ipu2_di1",
	"ahb",
	"ipg",
	"ipg_per",
	"ckil",
	"pll4_audio",
};

static struct clk_div_table clk_enet_ref_table[] = {
	{ .val = 0, .div = 20, },
	{ .val = 1, .div = 10, },
	{ .val = 2, .div = 5, },
	{ .val = 3, .div = 4, },
	{ },
};

static int imx6_ccm_probe(struct device_d *dev)
{
	void __iomem *base, *anatop_base, *ccm_base;
	unsigned long ckil_rate = 32768;
	unsigned long ckih_rate = 0;
	unsigned long osc_rate = 24000000;

	anatop_base = (void *)MX6_ANATOP_BASE_ADDR;
	ccm_base = dev_request_mem_region(dev, 0);

	base = anatop_base;

	clks[dummy] = clk_fixed("dummy", 0);
	clks[ckil] = clk_fixed("ckil", ckil_rate);
	clks[ckih] = clk_fixed("ckih", ckih_rate);
	clks[osc] = clk_fixed("osc", osc_rate);

	/*                   type                               name            parent_name base   div_mask */
	clks[pll1_sys]      = imx_clk_pllv3(IMX_PLLV3_SYS,	"pll1_sys",	"osc", base,        0x7f);
	clks[pll2_bus]      = imx_clk_pllv3(IMX_PLLV3_GENERIC,	"pll2_bus",	"osc", base + 0x30, 0x1);
	clks[pll3_usb_otg]  = imx_clk_pllv3(IMX_PLLV3_USB,	"pll3_usb_otg",	"osc", base + 0x10, 0x3);
	clks[pll4_audio]    = imx_clk_pllv3(IMX_PLLV3_AV,	"pll4_audio",	"osc", base + 0x70, 0x7f);
	clks[pll5_video]    = imx_clk_pllv3(IMX_PLLV3_AV,	"pll5_video",	"osc", base + 0xa0, 0x7f);
	clks[pll8_mlb]      = imx_clk_pllv3(IMX_PLLV3_MLB,	"pll8_mlb",	"osc", base + 0xd0, 0x0);
	clks[pll7_usb_host] = imx_clk_pllv3(IMX_PLLV3_USB,	"pll7_usb_host","osc", base + 0x20, 0x3);
	clks[pll6_enet]     = imx_clk_pllv3(IMX_PLLV3_ENET,	"pll6_enet",	"osc", base + 0xe0, 0x3);

	clks[usbphy1] = imx_clk_gate("usbphy1", "pll3_usb_otg", base + 0x10, 6);
	clks[usbphy2] = imx_clk_gate("usbphy2", "pll7_usb_host", base + 0x20, 6);

	clks[sata_ref] = imx_clk_fixed_factor("sata_ref", "pll6_enet", 1, 5);
	clks[pcie_ref] = imx_clk_fixed_factor("pcie_ref", "pll6_enet", 1, 4);
	clks[sata_ref_100m] = imx_clk_gate("sata_ref_100m", "sata_ref", base + 0xe0, 20);
	clks[pcie_ref_125m] = imx_clk_gate("pcie_ref_125m", "pcie_ref", base + 0xe0, 19);

	clks[enet_ref] = clk_divider_table("enet_ref", "pll6_enet", base + 0xe0, 0, 2, clk_enet_ref_table);

	/*                                name               parent_name         reg          idx */
	clks[pll2_pfd0_352m] = imx_clk_pfd("pll2_pfd0_352m", "pll2_bus",     base + 0x100, 0);
	clks[pll2_pfd1_594m] = imx_clk_pfd("pll2_pfd1_594m", "pll2_bus",     base + 0x100, 1);
	clks[pll2_pfd2_396m] = imx_clk_pfd("pll2_pfd2_396m", "pll2_bus",     base + 0x100, 2);
	clks[pll3_pfd0_720m] = imx_clk_pfd("pll3_pfd0_720m", "pll3_usb_otg", base + 0xf0,  0);
	clks[pll3_pfd1_540m] = imx_clk_pfd("pll3_pfd1_540m", "pll3_usb_otg", base + 0xf0,  1);
	clks[pll3_pfd2_508m] = imx_clk_pfd("pll3_pfd2_508m", "pll3_usb_otg", base + 0xf0,  2);
	clks[pll3_pfd3_454m] = imx_clk_pfd("pll3_pfd3_454m", "pll3_usb_otg", base + 0xf0,  3);

	/*                                    name          parent_name          mult div */
	clks[pll2_198m] = imx_clk_fixed_factor("pll2_198m", "pll2_pfd2_396m", 1, 2);
	clks[pll3_120m] = imx_clk_fixed_factor("pll3_120m", "pll3_usb_otg",   1, 4);
	clks[pll3_80m]  = imx_clk_fixed_factor("pll3_80m",  "pll3_usb_otg",   1, 6);
	clks[pll3_60m]  = imx_clk_fixed_factor("pll3_60m",  "pll3_usb_otg",   1, 8);
	clks[twd]       = imx_clk_fixed_factor("twd",       "arm",            1, 2);

	base = ccm_base;

	/*                                  name                 reg       shift width parent_names     num_parents */
	clks[step]             = imx_clk_mux("step",	         base + 0xc,  8,  1, step_sels,	        ARRAY_SIZE(step_sels));
	clks[pll1_sw]          = imx_clk_mux("pll1_sw",	         base + 0xc,  2,  1, pll1_sw_sels,      ARRAY_SIZE(pll1_sw_sels));
	clks[periph_pre]       = imx_clk_mux("periph_pre",       base + 0x18, 18, 2, periph_pre_sels,   ARRAY_SIZE(periph_pre_sels));
	clks[periph2_pre]      = imx_clk_mux("periph2_pre",      base + 0x18, 21, 2, periph_pre_sels,   ARRAY_SIZE(periph_pre_sels));
	clks[periph_clk2_sel]  = imx_clk_mux("periph_clk2_sel",  base + 0x18, 12, 1, periph_clk2_sels,  ARRAY_SIZE(periph_clk2_sels));
	clks[periph2_clk2_sel] = imx_clk_mux("periph2_clk2_sel", base + 0x18, 20, 1, periph_clk2_sels,  ARRAY_SIZE(periph_clk2_sels));
	clks[axi_sel]          = imx_clk_mux("axi_sel",          base + 0x14, 6,  2, axi_sels,          ARRAY_SIZE(axi_sels));
	clks[usdhc1_sel]       = imx_clk_mux("usdhc1_sel",       base + 0x1c, 16, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	clks[usdhc2_sel]       = imx_clk_mux("usdhc2_sel",       base + 0x1c, 17, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	clks[usdhc3_sel]       = imx_clk_mux("usdhc3_sel",       base + 0x1c, 18, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	clks[usdhc4_sel]       = imx_clk_mux("usdhc4_sel",       base + 0x1c, 19, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	clks[enfc_sel]         = imx_clk_mux("enfc_sel",         base + 0x2c, 16, 2, enfc_sels,         ARRAY_SIZE(enfc_sels));
	clks[emi_sel]          = imx_clk_mux("emi_sel",          base + 0x1c, 27, 2, emi_sels,          ARRAY_SIZE(emi_sels));
	clks[emi_slow_sel]     = imx_clk_mux("emi_slow_sel",     base + 0x1c, 29, 2, emi_sels,          ARRAY_SIZE(emi_sels));
	clks[vdo_axi_sel]      = imx_clk_mux("vdo_axi_sel",      base + 0x18, 11, 1, vdo_axi_sels,      ARRAY_SIZE(vdo_axi_sels));
	clks[cko1_sel]         = imx_clk_mux("cko1_sel",         base + 0x60, 0,  4, cko1_sels,         ARRAY_SIZE(cko1_sels));

	/*                              name         reg       shift width busy: reg, shift parent_names  num_parents */
	clks[periph]  = imx_clk_busy_mux("periph",  base + 0x14, 25,  1,   base + 0x48, 5,  periph_sels,  ARRAY_SIZE(periph_sels));
	clks[periph2] = imx_clk_busy_mux("periph2", base + 0x14, 26,  1,   base + 0x48, 3,  periph2_sels, ARRAY_SIZE(periph2_sels));

	/*                                      name                 parent_name               reg       shift width */
	clks[periph_clk2]      = imx_clk_divider("periph_clk2",      "periph_clk2_sel",   base + 0x14, 27, 3);
	clks[periph2_clk2]     = imx_clk_divider("periph2_clk2",     "periph2_clk2_sel",  base + 0x14, 0,  3);
	clks[ipg]              = imx_clk_divider("ipg",              "ahb",               base + 0x14, 8,  2);
	clks[ipg_per]          = imx_clk_divider("ipg_per",          "ipg",               base + 0x1c, 0,  6);
	clks[can_root]         = imx_clk_divider("can_root",         "pll3_usb_otg",      base + 0x20, 2,  6);
	clks[ecspi_root]       = imx_clk_divider("ecspi_root",       "pll3_60m",          base + 0x38, 19, 6);
	clks[uart_serial_podf] = imx_clk_divider("uart_serial_podf", "pll3_80m",          base + 0x24, 0,  6);
	clks[usdhc1_podf]      = imx_clk_divider("usdhc1_podf",      "usdhc1_sel",        base + 0x24, 11, 3);
	clks[usdhc2_podf]      = imx_clk_divider("usdhc2_podf",      "usdhc2_sel",        base + 0x24, 16, 3);
	clks[usdhc3_podf]      = imx_clk_divider("usdhc3_podf",      "usdhc3_sel",        base + 0x24, 19, 3);
	clks[usdhc4_podf]      = imx_clk_divider("usdhc4_podf",      "usdhc4_sel",        base + 0x24, 22, 3);
	clks[enfc_pred]        = imx_clk_divider("enfc_pred",        "enfc_sel",          base + 0x2c, 18, 3);
	clks[enfc_podf]        = imx_clk_divider("enfc_podf",        "enfc_pred",         base + 0x2c, 21, 6);
	clks[emi_podf]         = imx_clk_divider("emi_podf",         "emi_sel",           base + 0x1c, 20, 3);
	clks[emi_slow_podf]    = imx_clk_divider("emi_slow_podf",    "emi_slow_sel",      base + 0x1c, 23, 3);
	clks[cko1_podf]        = imx_clk_divider("cko1_podf",        "cko1_sel",          base + 0x60, 4,  3);

	/*                                            name                  parent_name         reg        shift width busy: reg, shift */
	clks[axi]               = imx_clk_busy_divider("axi",               "axi_sel",     base + 0x14, 16,  3,   base + 0x48, 0);
	clks[mmdc_ch0_axi_podf] = imx_clk_busy_divider("mmdc_ch0_axi_podf", "periph",      base + 0x14, 19,  3,   base + 0x48, 4);
	clks[mmdc_ch1_axi_podf] = imx_clk_busy_divider("mmdc_ch1_axi_podf", "periph2",     base + 0x14, 3,   3,   base + 0x48, 2);
	clks[arm]               = imx_clk_busy_divider("arm",               "pll1_sw",     base + 0x10, 0,   3,   base + 0x48, 16);
	clks[ahb]               = imx_clk_busy_divider("ahb",               "periph",      base + 0x14, 10,  3,   base + 0x48, 1);


	clkdev_add_physbase(clks[uart_serial_podf], MX6_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[uart_serial_podf], MX6_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[uart_serial_podf], MX6_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[uart_serial_podf], MX6_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[uart_serial_podf], MX6_UART5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ecspi_root], MX6_ECSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ecspi_root], MX6_ECSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ecspi_root], MX6_ECSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ecspi_root], MX6_ECSPI4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ecspi_root], MX6_ECSPI5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg_per], MX6_GPT_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX6_ENET_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usdhc1_podf], MX6_USDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usdhc2_podf], MX6_USDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usdhc3_podf], MX6_USDHC3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usdhc4_podf], MX6_USDHC4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg_per], MX6_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg_per], MX6_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg_per], MX6_I2C3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ahb], MX6_SATA_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usbphy1], MX6_USBPHY1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[usbphy2], MX6_USBPHY2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[enfc_podf], MX6_GPMI_BASE_ADDR, NULL);

	writel(0xffffffff, ccm_base + CCGR0);
	writel(0xffffffff, ccm_base + CCGR1);
	writel(0xffffffff, ccm_base + CCGR2);
	writel(0xffffffff, ccm_base + CCGR3);
	writel(0xffffffff, ccm_base + CCGR4);
	writel(0xffffffff, ccm_base + CCGR5);
	writel(0xffffffff, ccm_base + CCGR6);
	writel(0xffffffff, ccm_base + CCGR7);

	clk_enable(clks[pll6_enet]);
	clk_enable(clks[sata_ref_100m]);

	return 0;
}

static __maybe_unused struct of_device_id imx6_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx6q-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx6_ccm_driver = {
	.probe	= imx6_ccm_probe,
	.name	= "imx6-ccm",
	.of_compatible = DRV_OF_COMPAT(imx6_ccm_dt_ids),
};

static int imx6_ccm_init(void)
{
	return platform_driver_register(&imx6_ccm_driver);
}
core_initcall(imx6_ccm_init);
