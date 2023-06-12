// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <of.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <dt-bindings/clock/starfive-jh7100.h>

#include "clk.h"


static const char *cpundbus_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll0_out",
	[2] = "pll1_out",
	[3] = "pll2_out",
};

static const char *dla_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll1_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *dsp_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll0_out",
	[2] = "pll1_out",
	[3] = "pll2_out",
};

static const char *gmacusb_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll0_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *perh0_root_sels[2] = {
	[0] = "osc_sys",
	[1] = "pll0_out",
};

static const char *perh1_root_sels[2] = {
	[0] = "osc_sys",
	[1] = "pll2_out",
};

static const char *vin_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll1_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *vout_root_sels[4] = {
	[0] = "osc_aud",
	[1] = "pll0_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *cdechifi4_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll1_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *cdec_root_sels[4] = {
	[0] = "osc_sys",
	[1] = "pll0_out",
	[2] = "pll1_out",
	[3] = "dummy",
};

static const char *voutbus_root_sels[4] = {
	[0] = "osc_aud",
	[1] = "pll0_out",
	[2] = "pll2_out",
	[3] = "dummy",
};

static const char *pll2_refclk_sels[2] = {
	[0] = "osc_sys",
	[1] = "osc_aud",
};

static const char *ddrc0_sels[4] = {
	[0] = "ddrosc_div2",
	[1] = "ddrpll_div2",
	[2] = "ddrpll_div4",
	[3] = "ddrpll_div8",
};

static const char *ddrc1_sels[4] = {
	[0] = "ddrosc_div2",
	[1] = "ddrpll_div2",
	[2] = "ddrpll_div4",
	[3] = "ddrpll_div8",
};

static const char *nne_bus_sels[2] = {
	[0] = "cpu_axi",
	[1] = "nnebus_src1",
};

static const char *usbphy_25m_sels[2] = {
	[0] = "osc_sys",
	[1] = "usbphy_plldiv25m",
};

static const char *gmac_tx_sels[4] = {
	[0] = "gmac_gtxclk",
	[1] = "gmac_mii_txclk",
	[2] = "gmac_rmii_txclk",
	[3] = "dummy",
};

static const char *gmac_rx_pre_sels[2] = {
	[0] = "gmac_gr_mii_rxclk",
	[1] = "gmac_rmii_rxclk",
};

static struct clk *clks[CLK_END];

/* assume osc_sys as direct parent for clocks of yet unknown lineage */
#define UNKNOWN "osc_sys"

static void starfive_clkgen_init(struct device_node *np, void __iomem *base)
{
	clks[CLK_OSC_SYS]		= of_clk_get_by_name(np, "osc_sys");
	clks[CLK_OSC_AUD]		= of_clk_get_by_name(np, "osc_aud");
	clks[CLK_PLL0_OUT]		= starfive_clk_underspecifid("pll0_out", "osc_sys");
	clks[CLK_PLL1_OUT]		= starfive_clk_underspecifid("pll1_out", "osc_sys");
	clks[CLK_PLL2_OUT]		= starfive_clk_underspecifid("pll2_out", "pll2_refclk");
	clks[CLK_CPUNDBUS_ROOT]		= starfive_clk_mux("cpundbus_root", base + 0x0, 2, cpundbus_root_sels, ARRAY_SIZE(cpundbus_root_sels));
	clks[CLK_DLA_ROOT]		= starfive_clk_mux("dla_root",	base + 0x4, 2, dla_root_sels, ARRAY_SIZE(dla_root_sels));
	clks[CLK_DSP_ROOT]		= starfive_clk_mux("dsp_root",	base + 0x8, 2, dsp_root_sels, ARRAY_SIZE(dsp_root_sels));
	clks[CLK_GMACUSB_ROOT]		= starfive_clk_mux("gmacusb_root",	base + 0xC, 2, gmacusb_root_sels, ARRAY_SIZE(gmacusb_root_sels));
	clks[CLK_PERH0_ROOT]		= starfive_clk_mux("perh0_root",	base + 0x10, 1, perh0_root_sels, ARRAY_SIZE(perh0_root_sels));
	clks[CLK_PERH1_ROOT]		= starfive_clk_mux("perh1_root",	base + 0x14, 1, perh1_root_sels, ARRAY_SIZE(perh1_root_sels));
	clks[CLK_VIN_ROOT]		= starfive_clk_mux("vin_root",	base + 0x18, 2, vin_root_sels, ARRAY_SIZE(vin_root_sels));
	clks[CLK_VOUT_ROOT]		= starfive_clk_mux("vout_root",	base + 0x1C, 2, vout_root_sels, ARRAY_SIZE(vout_root_sels));
	clks[CLK_AUDIO_ROOT]		= starfive_clk_gated_divider("audio_root",		UNKNOWN,	base + 0x20, 4);
	clks[CLK_CDECHIFI4_ROOT]	= starfive_clk_mux("cdechifi4_root",	base + 0x24, 2, cdechifi4_root_sels, ARRAY_SIZE(cdechifi4_root_sels));
	clks[CLK_CDEC_ROOT]		= starfive_clk_mux("cdec_root",	base + 0x28, 2, cdec_root_sels, ARRAY_SIZE(cdec_root_sels));
	clks[CLK_VOUTBUS_ROOT]		= starfive_clk_mux("voutbus_root",	base + 0x2C, 2, voutbus_root_sels, ARRAY_SIZE(voutbus_root_sels));
	clks[CLK_CPUNBUS_ROOT_DIV]	= starfive_clk_divider("cpunbus_root_div",		"cpunbus_root",	base + 0x30, 2);
	clks[CLK_DSP_ROOT_DIV]		= starfive_clk_divider("dsp_root_div",		"dsp_root",	base + 0x34, 3);
	clks[CLK_PERH0_SRC]		= starfive_clk_divider("perh0_src",		"perh0_root",	base + 0x38, 3);
	clks[CLK_PERH1_SRC]		= starfive_clk_divider("perh1_src",		"perh1_root",	base + 0x3C, 3);
	clks[CLK_PLL0_TESTOUT]		= starfive_clk_gated_divider("pll0_testout",		"pll0_out",	base + 0x40, 5);
	clks[CLK_PLL1_TESTOUT]		= starfive_clk_gated_divider("pll1_testout",		"pll1_out",	base + 0x44, 5);
	clks[CLK_PLL2_TESTOUT]		= starfive_clk_gated_divider("pll2_testout",		"pll2_out",	base + 0x48, 5);
	clks[CLK_PLL2_REF]		= starfive_clk_mux("pll2_refclk",	base + 0x4C, 1, pll2_refclk_sels, ARRAY_SIZE(pll2_refclk_sels));
	clks[CLK_CPU_CORE]		= starfive_clk_divider("cpu_core",		UNKNOWN,	base + 0x50, 4);
	clks[CLK_CPU_AXI]		= starfive_clk_divider("cpu_axi",		UNKNOWN,	base + 0x54, 4);
	clks[CLK_AHB_BUS]		= starfive_clk_divider("ahb_bus",		UNKNOWN,	base + 0x58, 4);
	clks[CLK_APB1_BUS]		= starfive_clk_divider("apb1_bus",		UNKNOWN,	base + 0x5C, 4);
	clks[CLK_APB2_BUS]		= starfive_clk_divider("apb2_bus",		UNKNOWN,	base + 0x60, 4);
	clks[CLK_DOM3AHB_BUS]		= starfive_clk_gate("dom3ahb_bus",		UNKNOWN,	base + 0x64);
	clks[CLK_DOM7AHB_BUS]		= starfive_clk_gate("dom7ahb_bus",		UNKNOWN,	base + 0x68);
	clks[CLK_U74_CORE0]		= starfive_clk_gate("u74_core0",		UNKNOWN,	base + 0x6C);
	clks[CLK_U74_CORE1]		= starfive_clk_gated_divider("u74_core1",		"",	base + 0x70, 4);
	clks[CLK_U74_AXI]		= starfive_clk_gate("u74_axi",		UNKNOWN,	base + 0x74);
	clks[CLK_U74RTC_TOGGLE]		= starfive_clk_gate("u74rtc_toggle",		UNKNOWN,	base + 0x78);
	clks[CLK_SGDMA2P_AXI]		= starfive_clk_gate("sgdma2p_axi",		UNKNOWN,	base + 0x7C);
	clks[CLK_DMA2PNOC_AXI]		= starfive_clk_gate("dma2pnoc_axi",		UNKNOWN,	base + 0x80);
	clks[CLK_SGDMA2P_AHB]		= starfive_clk_gate("sgdma2p_ahb",		UNKNOWN,	base + 0x84);
	clks[CLK_DLA_BUS]		= starfive_clk_divider("dla_bus",		UNKNOWN,	base + 0x88, 3);
	clks[CLK_DLA_AXI]		= starfive_clk_gate("dla_axi",		UNKNOWN,	base + 0x8C);
	clks[CLK_DLANOC_AXI]		= starfive_clk_gate("dlanoc_axi",		UNKNOWN,	base + 0x90);
	clks[CLK_DLA_APB]		= starfive_clk_gate("dla_apb",		UNKNOWN,	base + 0x94);
	clks[CLK_VP6_CORE]		= starfive_clk_gated_divider("vp6_core",		UNKNOWN,	base + 0x98, 3);
	clks[CLK_VP6BUS_SRC]		= starfive_clk_divider("vp6bus_src",		UNKNOWN,	base + 0x9C, 3);
	clks[CLK_VP6_AXI]		= starfive_clk_gated_divider("vp6_axi",		UNKNOWN,	base + 0xA0, 3);
	clks[CLK_VCDECBUS_SRC]		= starfive_clk_divider("vcdecbus_src",		UNKNOWN,	base + 0xA4, 3);
	clks[CLK_VDEC_BUS]		= starfive_clk_divider("vdec_bus",		UNKNOWN,	base + 0xA8, 4);
	clks[CLK_VDEC_AXI]		= starfive_clk_gate("vdec_axi",		UNKNOWN,	base + 0xAC);
	clks[CLK_VDECBRG_MAIN]		= starfive_clk_gate("vdecbrg_mainclk",		UNKNOWN,	base + 0xB0);
	clks[CLK_VDEC_BCLK]		= starfive_clk_gated_divider("vdec_bclk",		UNKNOWN,	base + 0xB4, 4);
	clks[CLK_VDEC_CCLK]		= starfive_clk_gated_divider("vdec_cclk",		UNKNOWN,	base + 0xB8, 4);
	clks[CLK_VDEC_APB]		= starfive_clk_gate("vdec_apb",		UNKNOWN,	base + 0xBC);
	clks[CLK_JPEG_AXI]		= starfive_clk_gated_divider("jpeg_axi",		UNKNOWN,	base + 0xC0, 4);
	clks[CLK_JPEG_CCLK]		= starfive_clk_gated_divider("jpeg_cclk",		UNKNOWN,	base + 0xC4, 4);
	clks[CLK_JPEG_APB]		= starfive_clk_gate("jpeg_apb",		UNKNOWN,	base + 0xC8);
	clks[CLK_GC300_2X]		= starfive_clk_gated_divider("gc300_2x",		UNKNOWN,	base + 0xCC, 4);
	clks[CLK_GC300_AHB]		= starfive_clk_gate("gc300_ahb",		UNKNOWN,	base + 0xD0);
	clks[CLK_JPCGC300_AXIBUS]	= starfive_clk_divider("jpcgc300_axibus",		UNKNOWN,	base + 0xD4, 4);
	clks[CLK_GC300_AXI]		= starfive_clk_gate("gc300_axi",		UNKNOWN,	base + 0xD8);
	clks[CLK_JPCGC300_MAIN]		= starfive_clk_gate("jpcgc300_mainclk",		UNKNOWN,	base + 0xDC);
	clks[CLK_VENC_BUS]		= starfive_clk_divider("venc_bus",		UNKNOWN,	base + 0xE0, 4);
	clks[CLK_VENC_AXI]		= starfive_clk_gate("venc_axi",		UNKNOWN,	base + 0xE4);
	clks[CLK_VENCBRG_MAIN]		= starfive_clk_gate("vencbrg_mainclk",		UNKNOWN,	base + 0xE8);
	clks[CLK_VENC_BCLK]		= starfive_clk_gated_divider("venc_bclk",		UNKNOWN,	base + 0xEC, 4);
	clks[CLK_VENC_CCLK]		= starfive_clk_gated_divider("venc_cclk",		UNKNOWN,	base + 0xF0, 4);
	clks[CLK_VENC_APB]		= starfive_clk_gate("venc_apb",		UNKNOWN,	base + 0xF4);
	clks[CLK_DDRPLL_DIV2]		= starfive_clk_gated_divider("ddrpll_div2",		UNKNOWN,	base + 0xF8, 2);
	clks[CLK_DDRPLL_DIV4]		= starfive_clk_gated_divider("ddrpll_div4",		UNKNOWN,	base + 0xFC, 2);
	clks[CLK_DDRPLL_DIV8]		= starfive_clk_gated_divider("ddrpll_div8",		UNKNOWN,	base + 0x100, 2);
	clks[CLK_DDROSC_DIV2]		= starfive_clk_gated_divider("ddrosc_div2",		UNKNOWN,	base + 0x104, 2);
	clks[CLK_DDRC0]			= starfive_clk_mux("ddrc0",	base + 0x108, 2, ddrc0_sels, ARRAY_SIZE(ddrc0_sels));
	clks[CLK_DDRC1]			= starfive_clk_mux("ddrc1",	base + 0x10C, 2, ddrc1_sels, ARRAY_SIZE(ddrc1_sels));
	clks[CLK_DDRPHY_APB]		= starfive_clk_gate("ddrphy_apb",		UNKNOWN,	base + 0x110);
	clks[CLK_NOC_ROB]		= starfive_clk_divider("noc_rob",		UNKNOWN,	base + 0x114, 4);
	clks[CLK_NOC_COG]		= starfive_clk_divider("noc_cog",		UNKNOWN,	base + 0x118, 4);
	clks[CLK_NNE_AHB]		= starfive_clk_gate("nne_ahb",		UNKNOWN,	base + 0x11C);
	clks[CLK_NNEBUS_SRC1]		= starfive_clk_divider("nnebus_src1",		UNKNOWN,	base + 0x120, 3);
	clks[CLK_NNE_BUS]		= starfive_clk_mux("nne_bus",	base + 0x124, 2, nne_bus_sels, ARRAY_SIZE(nne_bus_sels));
	clks[CLK_NNE_AXI]		= starfive_clk_gate("nne_axi",	UNKNOWN, base + 0x128);
	clks[CLK_NNENOC_AXI]		= starfive_clk_gate("nnenoc_axi",		UNKNOWN,	base + 0x12C);
	clks[CLK_DLASLV_AXI]		= starfive_clk_gate("dlaslv_axi",		UNKNOWN,	base + 0x130);
	clks[CLK_DSPX2C_AXI]		= starfive_clk_gate("dspx2c_axi",		UNKNOWN,	base + 0x134);
	clks[CLK_HIFI4_SRC]		= starfive_clk_divider("hifi4_src",		UNKNOWN,	base + 0x138, 3);
	clks[CLK_HIFI4_COREFREE]	= starfive_clk_divider("hifi4_corefree",		UNKNOWN,	base + 0x13C, 4);
	clks[CLK_HIFI4_CORE]		= starfive_clk_gate("hifi4_core",		UNKNOWN,	base + 0x140);
	clks[CLK_HIFI4_BUS]		= starfive_clk_divider("hifi4_bus",		UNKNOWN,	base + 0x144, 4);
	clks[CLK_HIFI4_AXI]		= starfive_clk_gate("hifi4_axi",		UNKNOWN,	base + 0x148);
	clks[CLK_HIFI4NOC_AXI]		= starfive_clk_gate("hifi4noc_axi",		UNKNOWN,	base + 0x14C);
	clks[CLK_SGDMA1P_BUS]		= starfive_clk_divider("sgdma1p_bus",		UNKNOWN,	base + 0x150, 4);
	clks[CLK_SGDMA1P_AXI]		= starfive_clk_gate("sgdma1p_axi",		UNKNOWN,	base + 0x154);
	clks[CLK_DMA1P_AXI]		= starfive_clk_gate("dma1p_axi",		UNKNOWN,	base + 0x158);
	clks[CLK_X2C_AXI]		= starfive_clk_gated_divider("x2c_axi",		UNKNOWN,	base + 0x15C, 4);
	clks[CLK_USB_BUS]		= starfive_clk_divider("usb_bus",		UNKNOWN,	base + 0x160, 4);
	clks[CLK_USB_AXI]		= starfive_clk_gate("usb_axi",		UNKNOWN,	base + 0x164);
	clks[CLK_USBNOC_AXI]		= starfive_clk_gate("usbnoc_axi",		UNKNOWN,	base + 0x168);
	clks[CLK_USBPHY_ROOTDIV]	= starfive_clk_divider("usbphy_rootdiv",		UNKNOWN,	base + 0x16C, 3);
	clks[CLK_USBPHY_125M]		= starfive_clk_gated_divider("usbphy_125m",		UNKNOWN,	base + 0x170, 4);
	clks[CLK_USBPHY_PLLDIV25M]	= starfive_clk_gated_divider("usbphy_plldiv25m",		UNKNOWN,	base + 0x174, 6);
	clks[CLK_USBPHY_25M]		= starfive_clk_mux("usbphy_25m",	base + 0x178, 1, usbphy_25m_sels, ARRAY_SIZE(usbphy_25m_sels));
	clks[CLK_AUDIO_DIV]		= starfive_clk_divider("audio_div",		UNKNOWN,	base + 0x17C, 18);
	clks[CLK_AUDIO_SRC]		= starfive_clk_gate("audio_src",		UNKNOWN,	base + 0x180);
	clks[CLK_AUDIO_12288]		= starfive_clk_gate("audio_12288",		UNKNOWN,	base + 0x184);
	clks[CLK_VIN_SRC]		= starfive_clk_gated_divider("vin_src",		UNKNOWN,	base + 0x188, 3);
	clks[CLK_ISP0_BUS]		= starfive_clk_divider("isp0_bus",		UNKNOWN,	base + 0x18C, 4);
	clks[CLK_ISP0_AXI]		= starfive_clk_gate("isp0_axi",		UNKNOWN,	base + 0x190);
	clks[CLK_ISP0NOC_AXI]		= starfive_clk_gate("isp0noc_axi",		UNKNOWN,	base + 0x194);
	clks[CLK_ISPSLV_AXI]		= starfive_clk_gate("ispslv_axi",		UNKNOWN,	base + 0x198);
	clks[CLK_ISP1_BUS]		= starfive_clk_divider("isp1_bus",		UNKNOWN,	base + 0x19C, 4);
	clks[CLK_ISP1_AXI]		= starfive_clk_gate("isp1_axi",		UNKNOWN,	base + 0x1A0);
	clks[CLK_ISP1NOC_AXI]		= starfive_clk_gate("isp1noc_axi",		UNKNOWN,	base + 0x1A4);
	clks[CLK_VIN_BUS]		= starfive_clk_divider("vin_bus",		UNKNOWN,	base + 0x1A8, 4);
	clks[CLK_VIN_AXI]		= starfive_clk_gate("vin_axi",		UNKNOWN,	base + 0x1AC);
	clks[CLK_VINNOC_AXI]		= starfive_clk_gate("vinnoc_axi",		UNKNOWN,	base + 0x1B0);
	clks[CLK_VOUT_SRC]		= starfive_clk_gated_divider("vout_src",		UNKNOWN,	base + 0x1B4, 3);
	clks[CLK_DISPBUS_SRC]		= starfive_clk_divider("dispbus_src",		UNKNOWN,	base + 0x1B8, 3);
	clks[CLK_DISP_BUS]		= starfive_clk_divider("disp_bus",		UNKNOWN,	base + 0x1BC, 3);
	clks[CLK_DISP_AXI]		= starfive_clk_gate("disp_axi",		UNKNOWN,	base + 0x1C0);
	clks[CLK_DISPNOC_AXI]		= starfive_clk_gate("dispnoc_axi",		UNKNOWN,	base + 0x1C4);
	clks[CLK_SDIO0_AHB]		= starfive_clk_gate("sdio0_ahb",		UNKNOWN,	base + 0x1C8);
	clks[CLK_SDIO0_CCLKINT]		= starfive_clk_gated_divider("sdio0_cclkint",		UNKNOWN,	base + 0x1CC, 5);
	clks[CLK_SDIO0_CCLKINT_INV]	= starfive_clk_gate_dis("sdio0_cclkint_inv",		UNKNOWN,	base + 0x1D0);
	clks[CLK_SDIO1_AHB]		= starfive_clk_gate("sdio1_ahb",		UNKNOWN,	base + 0x1D4);
	clks[CLK_SDIO1_CCLKINT]		= starfive_clk_gated_divider("sdio1_cclkint",		UNKNOWN,	base + 0x1D8, 5);
	clks[CLK_SDIO1_CCLKINT_INV]	= starfive_clk_gate_dis("sdio1_cclkint_inv",		UNKNOWN,	base + 0x1DC);
	clks[CLK_GMAC_AHB]		= starfive_clk_gate("gmac_ahb",		UNKNOWN,	base + 0x1E0);
	clks[CLK_GMAC_ROOT_DIV]		= starfive_clk_divider("gmac_root_div",		UNKNOWN,	base + 0x1E4, 4);
	clks[CLK_GMAC_PTP_REF]		= starfive_clk_gated_divider("gmac_ptp_refclk",		UNKNOWN,	base + 0x1E8, 5);
	clks[CLK_GMAC_GTX]		= starfive_clk_gated_divider("gmac_gtxclk",		UNKNOWN,	base + 0x1EC, 8);
	clks[CLK_GMAC_RMII_TX]		= starfive_clk_gated_divider("gmac_rmii_txclk",		UNKNOWN,	base + 0x1F0, 4);
	clks[CLK_GMAC_RMII_RX]		= starfive_clk_gated_divider("gmac_rmii_rxclk",		UNKNOWN,	base + 0x1F4, 4);
	clks[CLK_GMAC_TX]		= starfive_clk_mux("gmac_tx",	base + 0x1F8, 2, gmac_tx_sels, ARRAY_SIZE(gmac_tx_sels));
	clks[CLK_GMAC_TX_INV]		= starfive_clk_gate_dis("gmac_tx_inv",		UNKNOWN,	base + 0x1FC);
	clks[CLK_GMAC_RX_PRE]		= starfive_clk_mux("gmac_rx_pre",	base + 0x200, 1, gmac_rx_pre_sels, ARRAY_SIZE(gmac_rx_pre_sels));
	clks[CLK_GMAC_RX_INV]		= starfive_clk_gate_dis("gmac_rx_inv",		UNKNOWN,	base + 0x204);
	clks[CLK_GMAC_RMII]		= starfive_clk_gate("gmac_rmii",		UNKNOWN,	base + 0x208);
	clks[CLK_GMAC_TOPHYREF]		= starfive_clk_gated_divider("gmac_tophyref",		UNKNOWN,	base + 0x20C, 7);
	clks[CLK_SPI2AHB_AHB]		= starfive_clk_gate("spi2ahb_ahb",		UNKNOWN,	base + 0x210);
	clks[CLK_SPI2AHB_CORE]		= starfive_clk_gated_divider("spi2ahb_core",		UNKNOWN,	base + 0x214, 5);
	clks[CLK_EZMASTER_AHB]		= starfive_clk_gate("ezmaster_ahb",		UNKNOWN,	base + 0x218);
	clks[CLK_E24_AHB]		= starfive_clk_gate("e24_ahb",		UNKNOWN,	base + 0x21C);
	clks[CLK_E24RTC_TOGGLE]		= starfive_clk_gate("e24rtc_toggle",		UNKNOWN,	base + 0x220);
	clks[CLK_QSPI_AHB]		= starfive_clk_gate("qspi_ahb",		UNKNOWN,	base + 0x224);
	clks[CLK_QSPI_APB]		= starfive_clk_gate("qspi_apb",		UNKNOWN,	base + 0x228);
	clks[CLK_QSPI_REF]		= starfive_clk_gated_divider("qspi_refclk",		UNKNOWN,	base + 0x22C, 5);
	clks[CLK_SEC_AHB]		= starfive_clk_gate("sec_ahb",		UNKNOWN,	base + 0x230);
	clks[CLK_AES]			= starfive_clk_gate("aes_clk",		UNKNOWN,	base + 0x234);
	clks[CLK_SHA]			= starfive_clk_gate("sha_clk",		UNKNOWN,	base + 0x238);
	clks[CLK_PKA]			= starfive_clk_gate("pka_clk",		UNKNOWN,	base + 0x23C);
	clks[CLK_TRNG_APB]		= starfive_clk_gate("trng_apb",		UNKNOWN,	base + 0x240);
	clks[CLK_OTP_APB]		= starfive_clk_gate("otp_apb",		UNKNOWN,	base + 0x244);
	clks[CLK_UART0_APB]		= starfive_clk_gate("uart0_apb",		UNKNOWN,	base + 0x248);
	clks[CLK_UART0_CORE]		= starfive_clk_gated_divider("uart0_core",		UNKNOWN,	base + 0x24C, 6);
	clks[CLK_UART1_APB]		= starfive_clk_gate("uart1_apb",		UNKNOWN,	base + 0x250);
	clks[CLK_UART1_CORE]		= starfive_clk_gated_divider("uart1_core",		UNKNOWN,	base + 0x254, 6);
	clks[CLK_SPI0_APB]		= starfive_clk_gate("spi0_apb",		UNKNOWN,	base + 0x258);
	clks[CLK_SPI0_CORE]		= starfive_clk_gated_divider("spi0_core",		UNKNOWN,	base + 0x25C, 6);
	clks[CLK_SPI1_APB]		= starfive_clk_gate("spi1_apb",		UNKNOWN,	base + 0x260);
	clks[CLK_SPI1_CORE]		= starfive_clk_gated_divider("spi1_core",		UNKNOWN,	base + 0x264, 6);
	clks[CLK_I2C0_APB]		= starfive_clk_gate("i2c0_apb",		UNKNOWN,	base + 0x268);
	clks[CLK_I2C0_CORE]		= starfive_clk_gated_divider("i2c0_core",		UNKNOWN,	base + 0x26C, 6);
	clks[CLK_I2C1_APB]		= starfive_clk_gate("i2c1_apb",		UNKNOWN,	base + 0x270);
	clks[CLK_I2C1_CORE]		= starfive_clk_gated_divider("i2c1_core",		UNKNOWN,	base + 0x274, 6);
	clks[CLK_GPIO_APB]		= starfive_clk_gate("gpio_apb",		UNKNOWN,	base + 0x278);
	clks[CLK_UART2_APB]		= starfive_clk_gate("uart2_apb",		UNKNOWN,	base + 0x27C);
	clks[CLK_UART2_CORE]		= starfive_clk_gated_divider("uart2_core",		UNKNOWN,	base + 0x280, 6);
	clks[CLK_UART3_APB]		= starfive_clk_gate("uart3_apb",		UNKNOWN,	base + 0x284);
	clks[CLK_UART3_CORE]		= starfive_clk_gated_divider("uart3_core",		UNKNOWN,	base + 0x288, 6);
	clks[CLK_SPI2_APB]		= starfive_clk_gate("spi2_apb",		UNKNOWN,	base + 0x28C);
	clks[CLK_SPI2_CORE]		= starfive_clk_gated_divider("spi2_core",		UNKNOWN,	base + 0x290, 6);
	clks[CLK_SPI3_APB]		= starfive_clk_gate("spi3_apb",		UNKNOWN,	base + 0x294);
	clks[CLK_SPI3_CORE]		= starfive_clk_gated_divider("spi3_core",		UNKNOWN,	base + 0x298, 6);
	clks[CLK_I2C2_APB]		= starfive_clk_gate("i2c2_apb",		UNKNOWN,	base + 0x29C);
	clks[CLK_I2C2_CORE]		= starfive_clk_gated_divider("i2c2_core",		UNKNOWN,	base + 0x2A0, 6);
	clks[CLK_I2C3_APB]		= starfive_clk_gate("i2c3_apb",		UNKNOWN,	base + 0x2A4);
	clks[CLK_I2C3_CORE]		= starfive_clk_gated_divider("i2c3_core",		UNKNOWN,	base + 0x2A8, 6);
	clks[CLK_WDTIMER_APB]		= starfive_clk_gate("wdtimer_apb",		UNKNOWN,	base + 0x2AC);
	clks[CLK_WDT_CORE]		= starfive_clk_gated_divider("wdt_coreclk",		UNKNOWN,	base + 0x2B0, 6);
	clks[CLK_TIMER0_CORE]		= starfive_clk_gated_divider("timer0_coreclk",		UNKNOWN,	base + 0x2B4, 6);
	clks[CLK_TIMER1_CORE]		= starfive_clk_gated_divider("timer1_coreclk",		UNKNOWN,	base + 0x2B8, 6);
	clks[CLK_TIMER2_CORE]		= starfive_clk_gated_divider("timer2_coreclk",		UNKNOWN,	base + 0x2BC, 6);
	clks[CLK_TIMER3_CORE]		= starfive_clk_gated_divider("timer3_coreclk",		UNKNOWN,	base + 0x2C0, 6);
	clks[CLK_TIMER4_CORE]		= starfive_clk_gated_divider("timer4_coreclk",		UNKNOWN,	base + 0x2C4, 6);
	clks[CLK_TIMER5_CORE]		= starfive_clk_gated_divider("timer5_coreclk",		UNKNOWN,	base + 0x2C8, 6);
	clks[CLK_TIMER6_CORE]		= starfive_clk_gated_divider("timer6_coreclk",		UNKNOWN,	base + 0x2CC, 6);
	clks[CLK_VP6INTC_APB]		= starfive_clk_gate("vp6intc_apb",		UNKNOWN,	base + 0x2D0);
	clks[CLK_PWM_APB]		= starfive_clk_gate("pwm_apb",		UNKNOWN,	base + 0x2D4);
	clks[CLK_MSI_APB]		= starfive_clk_gate("msi_apb",		UNKNOWN,	base + 0x2D8);
	clks[CLK_TEMP_APB]		= starfive_clk_gate("temp_apb",		UNKNOWN,	base + 0x2DC);
	clks[CLK_TEMP_SENSE]		= starfive_clk_gated_divider("temp_sense",		UNKNOWN,	base + 0x2E0, 5);
	clks[CLK_SYSERR_APB]		= starfive_clk_gate("syserr_apb",		UNKNOWN,	base + 0x2E4);
}

static struct clk_onecell_data clk_data;

static int starfive_clkgen_clk_probe(struct device *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	starfive_clkgen_init(dev->of_node, IOMEM(iores->start));

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id starfive_clkgen_clk_dt_ids[] = {
	{ .compatible = "starfive,jh7100-clkgen" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, starfive_clkgen_clk_dt_ids);

static struct driver starfive_clkgen_clk_driver = {
	.probe	= starfive_clkgen_clk_probe,
	.name	= "starfive-clkgen",
	.of_compatible = starfive_clkgen_clk_dt_ids,
};
core_platform_driver(starfive_clkgen_clk_driver);
