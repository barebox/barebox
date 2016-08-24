/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Designware High-Definition Multimedia Interface (HDMI) driver
 *
 * Copyright (C) 2010, Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 */
#include <common.h>
#include <fb.h>
#include <io.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>
#include <linux/clk.h>
#include <i2c/i2c.h>
#include <video/media-bus-format.h>
#include <video/vpl.h>
#include <mach/imx6-regs.h>
#include <mach/imx53-regs.h>

#include "imx-ipu-v3.h"
#include "ipuv3-plane.h"
#include "imx-hdmi.h"

#define HDMI_EDID_LEN		512

#define RGB			0
#define YCBCR444		1
#define YCBCR422_16BITS		2
#define YCBCR422_8BITS		3
#define XVYCC444		4

enum hdmi_colorimetry {
	HDMI_COLORIMETRY_NONE,
	HDMI_COLORIMETRY_ITU_601,
	HDMI_COLORIMETRY_ITU_709,
	HDMI_COLORIMETRY_EXTENDED,
};

enum hdmi_datamap {
	RGB444_8B = 0x01,
	RGB444_10B = 0x03,
	RGB444_12B = 0x05,
	RGB444_16B = 0x07,
	YCbCr444_8B = 0x09,
	YCbCr444_10B = 0x0B,
	YCbCr444_12B = 0x0D,
	YCbCr444_16B = 0x0F,
	YCbCr422_8B = 0x16,
	YCbCr422_10B = 0x14,
	YCbCr422_12B = 0x12,
};

enum dw_hdmi_devtype {
	IMX6Q_HDMI,
	IMX6DL_HDMI,
};

static const u16 csc_coeff_default[3][4] = {
	{ 0x2000, 0x0000, 0x0000, 0x0000 },
	{ 0x0000, 0x2000, 0x0000, 0x0000 },
	{ 0x0000, 0x0000, 0x2000, 0x0000 }
};

static const u16 csc_coeff_rgb_out_eitu601[3][4] = {
	{ 0x2000, 0x6926, 0x74fd, 0x010e },
	{ 0x2000, 0x2cdd, 0x0000, 0x7e9a },
	{ 0x2000, 0x0000, 0x38b4, 0x7e3b }
};

static const u16 csc_coeff_rgb_out_eitu709[3][4] = {
	{ 0x2000, 0x7106, 0x7a02, 0x00a7 },
	{ 0x2000, 0x3264, 0x0000, 0x7e6d },
	{ 0x2000, 0x0000, 0x3b61, 0x7e25 }
};

static const u16 csc_coeff_rgb_in_eitu601[3][4] = {
	{ 0x2591, 0x1322, 0x074b, 0x0000 },
	{ 0x6535, 0x2000, 0x7acc, 0x0200 },
	{ 0x6acd, 0x7534, 0x2000, 0x0200 }
};

static const u16 csc_coeff_rgb_in_eitu709[3][4] = {
	{ 0x2dc5, 0x0d9b, 0x049e, 0x0000 },
	{ 0x62f0, 0x2000, 0x7d11, 0x0200 },
	{ 0x6756, 0x78ab, 0x2000, 0x0200 }
};

struct hdmi_vmode {
	bool mdvi;
	bool mhsyncpolarity;
	bool mvsyncpolarity;
	bool minterlaced;
	bool mdataenablepolarity;

	unsigned int mpixelclock;
	unsigned int mpixelrepetitioninput;
	unsigned int mpixelrepetitionoutput;
};

struct hdmi_data_info {
	unsigned int enc_in_format;
	unsigned int enc_out_format;
	unsigned int enc_color_depth;
	unsigned int colorimetry;
	unsigned int pix_repet_factor;
	unsigned int hdcp_enable;
	struct hdmi_vmode video_mode;
};

struct dw_hdmi {
	enum dw_hdmi_devtype dev_type;
	struct device_d *dev;
	struct clk *isfr_clk;
	struct clk *iahb_clk;

	bool connected;

	struct hdmi_data_info hdmi_data;
	int vic;

	u8 edid[HDMI_EDID_LEN];
	bool cable_plugin;

	bool phy_enabled;

	struct device_node *ddc_node;;
	void __iomem *regs;

	unsigned int sample_rate;
	int ratio;

	struct fb_videomode *mode;

	struct vpl vpl;
};

static void dw_hdmi_set_ipu_di_mux(struct dw_hdmi *hdmi, int ipu_di)
{
	void __iomem *gpr3 = (void *)MX6_IOMUXC_BASE_ADDR + 0xc;
	uint32_t val;

	dev_info(hdmi->dev, "setup hdmi mux to %d\n", ipu_di);

	val = readl(gpr3);
	val &= ~(3 << 2);
	val |= ipu_di << 2;
	writel(val, gpr3);
}

static inline void hdmi_writeb(struct dw_hdmi *hdmi, u8 val, int offset)
{
	writeb(val, hdmi->regs + offset);
}

static inline u8 hdmi_readb(struct dw_hdmi *hdmi, int offset)
{
	return readb(hdmi->regs + offset);
}

static void hdmi_modb(struct dw_hdmi *hdmi, u8 data, u8 mask, unsigned reg)
{
	u8 val = hdmi_readb(hdmi, reg) & ~mask;

	val |= data & mask;
	hdmi_writeb(hdmi, val, reg);
}

static void hdmi_mask_writeb(struct dw_hdmi *hdmi, u8 data, unsigned int reg,
			     u8 shift, u8 mask)
{
	hdmi_modb(hdmi, data << shift, mask, reg);
}

static void hdmi_set_cts_n(struct dw_hdmi *hdmi, unsigned int cts,
			   unsigned int n)
{
	/* Must be set/cleared first */
	hdmi_modb(hdmi, 0, HDMI_AUD_CTS3_CTS_MANUAL, HDMI_AUD_CTS3);

	/* nshift factor = 0 */
	hdmi_modb(hdmi, 0, HDMI_AUD_CTS3_N_SHIFT_MASK, HDMI_AUD_CTS3);

	hdmi_writeb(hdmi, ((cts >> 16) & HDMI_AUD_CTS3_AUDCTS19_16_MASK) |
		    HDMI_AUD_CTS3_CTS_MANUAL, HDMI_AUD_CTS3);
	hdmi_writeb(hdmi, (cts >> 8) & 0xff, HDMI_AUD_CTS2);
	hdmi_writeb(hdmi, cts & 0xff, HDMI_AUD_CTS1);

	hdmi_writeb(hdmi, (n >> 16) & 0x0f, HDMI_AUD_N3);
	hdmi_writeb(hdmi, (n >> 8) & 0xff, HDMI_AUD_N2);
	hdmi_writeb(hdmi, n & 0xff, HDMI_AUD_N1);
}

static unsigned int hdmi_compute_n(unsigned int freq, unsigned long pixel_clk,
				   unsigned int ratio)
{
	unsigned int n = (128 * freq) / 1000;

	switch (freq) {
	case 32000:
		if (pixel_clk == 25170000)
			n = (ratio == 150) ? 9152 : 4576;
		else if (pixel_clk == 27020000)
			n = (ratio == 150) ? 8192 : 4096;
		else if (pixel_clk == 74170000 || pixel_clk == 148350000)
			n = 11648;
		else
			n = 4096;
		break;

	case 44100:
		if (pixel_clk == 25170000)
			n = 7007;
		else if (pixel_clk == 74170000)
			n = 17836;
		else if (pixel_clk == 148350000)
			n = (ratio == 150) ? 17836 : 8918;
		else
			n = 6272;
		break;

	case 48000:
		if (pixel_clk == 25170000)
			n = (ratio == 150) ? 9152 : 6864;
		else if (pixel_clk == 27020000)
			n = (ratio == 150) ? 8192 : 6144;
		else if (pixel_clk == 74170000)
			n = 11648;
		else if (pixel_clk == 148350000)
			n = (ratio == 150) ? 11648 : 5824;
		else
			n = 6144;
		break;

	case 88200:
		n = hdmi_compute_n(44100, pixel_clk, ratio) * 2;
		break;

	case 96000:
		n = hdmi_compute_n(48000, pixel_clk, ratio) * 2;
		break;

	case 176400:
		n = hdmi_compute_n(44100, pixel_clk, ratio) * 4;
		break;

	case 192000:
		n = hdmi_compute_n(48000, pixel_clk, ratio) * 4;
		break;

	default:
		break;
	}

	return n;
}

static unsigned int hdmi_compute_cts(unsigned int freq, unsigned long pixel_clk,
				     unsigned int ratio)
{
	unsigned int cts = 0;

	pr_debug("%s: freq: %d pixel_clk: %ld ratio: %d\n", __func__, freq,
		 pixel_clk, ratio);

	switch (freq) {
	case 32000:
		if (pixel_clk == 297000000) {
			cts = 222750;
			break;
		}
	case 48000:
	case 96000:
	case 192000:
		switch (pixel_clk) {
		case 25200000:
		case 27000000:
		case 54000000:
		case 74250000:
		case 148500000:
			cts = pixel_clk / 1000;
			break;
		case 297000000:
			cts = 247500;
			break;
		/*
		 * All other TMDS clocks are not supported by
		 * DWC_hdmi_tx. The TMDS clocks divided or
		 * multiplied by 1,001 coefficients are not
		 * supported.
		 */
		default:
			break;
		}
		break;
	case 44100:
	case 88200:
	case 176400:
		switch (pixel_clk) {
		case 25200000:
			cts = 28000;
			break;
		case 27000000:
			cts = 30000;
			break;
		case 54000000:
			cts = 60000;
			break;
		case 74250000:
			cts = 82500;
			break;
		case 148500000:
			cts = 165000;
			break;
		case 297000000:
			cts = 247500;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	if (ratio == 100)
		return cts;
	return (cts * ratio) / 100;
}

static void hdmi_set_clk_regenerator(struct dw_hdmi *hdmi,
				     unsigned long pixel_clk)
{
	unsigned int clk_n, clk_cts;

	clk_n = hdmi_compute_n(hdmi->sample_rate, pixel_clk,
			       hdmi->ratio);
	clk_cts = hdmi_compute_cts(hdmi->sample_rate, pixel_clk,
				   hdmi->ratio);

	if (!clk_cts) {
		dev_dbg(hdmi->dev, "%s: pixel clock not supported: %lu\n",
			__func__, pixel_clk);
		return;
	}

	dev_dbg(hdmi->dev, "%s: samplerate=%d  ratio=%d  pixelclk=%lu  N=%d cts=%d\n",
		__func__, hdmi->sample_rate, hdmi->ratio,
		pixel_clk, clk_n, clk_cts);

	hdmi_set_cts_n(hdmi, clk_cts, clk_n);
}

static void hdmi_init_clk_regenerator(struct dw_hdmi *hdmi)
{
	hdmi_set_clk_regenerator(hdmi, 74250000);
}

/*
 * this submodule is responsible for the video data synchronization.
 * for example, for RGB 4:4:4 input, the data map is defined as
 *			pin{47~40} <==> R[7:0]
 *			pin{31~24} <==> G[7:0]
 *			pin{15~8}  <==> B[7:0]
 */
static void hdmi_video_sample(struct dw_hdmi *hdmi)
{
	int color_format = 0;
	u8 val;

	if (hdmi->hdmi_data.enc_in_format == RGB) {
		if (hdmi->hdmi_data.enc_color_depth == 8)
			color_format = 0x01;
		else if (hdmi->hdmi_data.enc_color_depth == 10)
			color_format = 0x03;
		else if (hdmi->hdmi_data.enc_color_depth == 12)
			color_format = 0x05;
		else if (hdmi->hdmi_data.enc_color_depth == 16)
			color_format = 0x07;
		else
			return;
	} else if (hdmi->hdmi_data.enc_in_format == YCBCR444) {
		if (hdmi->hdmi_data.enc_color_depth == 8)
			color_format = 0x09;
		else if (hdmi->hdmi_data.enc_color_depth == 10)
			color_format = 0x0B;
		else if (hdmi->hdmi_data.enc_color_depth == 12)
			color_format = 0x0D;
		else if (hdmi->hdmi_data.enc_color_depth == 16)
			color_format = 0x0F;
		else
			return;
	} else if (hdmi->hdmi_data.enc_in_format == YCBCR422_8BITS) {
		if (hdmi->hdmi_data.enc_color_depth == 8)
			color_format = 0x16;
		else if (hdmi->hdmi_data.enc_color_depth == 10)
			color_format = 0x14;
		else if (hdmi->hdmi_data.enc_color_depth == 12)
			color_format = 0x12;
		else
			return;
	}

	val = HDMI_TX_INVID0_INTERNAL_DE_GENERATOR_DISABLE |
		((color_format << HDMI_TX_INVID0_VIDEO_MAPPING_OFFSET) &
		HDMI_TX_INVID0_VIDEO_MAPPING_MASK);
	hdmi_writeb(hdmi, val, HDMI_TX_INVID0);

	/* Enable TX stuffing: When DE is inactive, fix the output data to 0 */
	val = HDMI_TX_INSTUFFING_BDBDATA_STUFFING_ENABLE |
		HDMI_TX_INSTUFFING_RCRDATA_STUFFING_ENABLE |
		HDMI_TX_INSTUFFING_GYDATA_STUFFING_ENABLE;
	hdmi_writeb(hdmi, val, HDMI_TX_INSTUFFING);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_GYDATA0);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_GYDATA1);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_RCRDATA0);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_RCRDATA1);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_BCBDATA0);
	hdmi_writeb(hdmi, 0x0, HDMI_TX_BCBDATA1);
}

static int is_color_space_conversion(struct dw_hdmi *hdmi)
{
	return hdmi->hdmi_data.enc_in_format != hdmi->hdmi_data.enc_out_format;
}

static int is_color_space_decimation(struct dw_hdmi *hdmi)
{
	if (hdmi->hdmi_data.enc_out_format != YCBCR422_8BITS)
		return 0;
	if (hdmi->hdmi_data.enc_in_format == RGB ||
	    hdmi->hdmi_data.enc_in_format == YCBCR444)
		return 1;
	return 0;
}

static int is_color_space_interpolation(struct dw_hdmi *hdmi)
{
	if (hdmi->hdmi_data.enc_in_format != YCBCR422_8BITS)
		return 0;
	if (hdmi->hdmi_data.enc_out_format == RGB ||
	    hdmi->hdmi_data.enc_out_format == YCBCR444)
		return 1;
	return 0;
}

static void dw_hdmi_update_csc_coeffs(struct dw_hdmi *hdmi)
{
	const u16 (*csc_coeff)[3][4] = &csc_coeff_default;
	unsigned i;
	u32 csc_scale = 1;

	if (is_color_space_conversion(hdmi)) {
		if (hdmi->hdmi_data.enc_out_format == RGB) {
			if (hdmi->hdmi_data.colorimetry ==
					HDMI_COLORIMETRY_ITU_601)
				csc_coeff = &csc_coeff_rgb_out_eitu601;
			else
				csc_coeff = &csc_coeff_rgb_out_eitu709;
		} else if (hdmi->hdmi_data.enc_in_format == RGB) {
			if (hdmi->hdmi_data.colorimetry ==
					HDMI_COLORIMETRY_ITU_601)
				csc_coeff = &csc_coeff_rgb_in_eitu601;
			else
				csc_coeff = &csc_coeff_rgb_in_eitu709;
			csc_scale = 0;
		}
	}

	/* The CSC registers are sequential, alternating MSB then LSB */
	for (i = 0; i < ARRAY_SIZE(csc_coeff_default[0]); i++) {
		u16 coeff_a = (*csc_coeff)[0][i];
		u16 coeff_b = (*csc_coeff)[1][i];
		u16 coeff_c = (*csc_coeff)[2][i];

		hdmi_writeb(hdmi, coeff_a & 0xff, HDMI_CSC_COEF_A1_LSB + i * 2);
		hdmi_writeb(hdmi, coeff_a >> 8, HDMI_CSC_COEF_A1_MSB + i * 2);
		hdmi_writeb(hdmi, coeff_b & 0xff, HDMI_CSC_COEF_B1_LSB + i * 2);
		hdmi_writeb(hdmi, coeff_b >> 8, HDMI_CSC_COEF_B1_MSB + i * 2);
		hdmi_writeb(hdmi, coeff_c & 0xff, HDMI_CSC_COEF_C1_LSB + i * 2);
		hdmi_writeb(hdmi, coeff_c >> 8, HDMI_CSC_COEF_C1_MSB + i * 2);
	}

	hdmi_modb(hdmi, csc_scale, HDMI_CSC_SCALE_CSCSCALE_MASK,
		  HDMI_CSC_SCALE);
}

static void hdmi_video_csc(struct dw_hdmi *hdmi)
{
	int color_depth = 0;
	int interpolation = HDMI_CSC_CFG_INTMODE_DISABLE;
	int decimation = 0;

	/* YCC422 interpolation to 444 mode */
	if (is_color_space_interpolation(hdmi))
		interpolation = HDMI_CSC_CFG_INTMODE_CHROMA_INT_FORMULA1;
	else if (is_color_space_decimation(hdmi))
		decimation = HDMI_CSC_CFG_DECMODE_CHROMA_INT_FORMULA3;

	if (hdmi->hdmi_data.enc_color_depth == 8)
		color_depth = HDMI_CSC_SCALE_CSC_COLORDE_PTH_24BPP;
	else if (hdmi->hdmi_data.enc_color_depth == 10)
		color_depth = HDMI_CSC_SCALE_CSC_COLORDE_PTH_30BPP;
	else if (hdmi->hdmi_data.enc_color_depth == 12)
		color_depth = HDMI_CSC_SCALE_CSC_COLORDE_PTH_36BPP;
	else if (hdmi->hdmi_data.enc_color_depth == 16)
		color_depth = HDMI_CSC_SCALE_CSC_COLORDE_PTH_48BPP;
	else
		return;

	/* Configure the CSC registers */
	hdmi_writeb(hdmi, interpolation | decimation, HDMI_CSC_CFG);
	hdmi_modb(hdmi, color_depth, HDMI_CSC_SCALE_CSC_COLORDE_PTH_MASK,
		  HDMI_CSC_SCALE);

	dw_hdmi_update_csc_coeffs(hdmi);
}

/*
 * HDMI video packetizer is used to packetize the data.
 * for example, if input is YCC422 mode or repeater is used,
 * data should be repacked this module can be bypassed.
 */
static void hdmi_video_packetize(struct dw_hdmi *hdmi)
{
	unsigned int color_depth = 0;
	unsigned int remap_size = HDMI_VP_REMAP_YCC422_16bit;
	unsigned int output_select = HDMI_VP_CONF_OUTPUT_SELECTOR_PP;
	struct hdmi_data_info *hdmi_data = &hdmi->hdmi_data;
	u8 val, vp_conf;

	if (hdmi_data->enc_out_format == RGB ||
	    hdmi_data->enc_out_format == YCBCR444) {
		if (!hdmi_data->enc_color_depth) {
			output_select = HDMI_VP_CONF_OUTPUT_SELECTOR_BYPASS;
		} else if (hdmi_data->enc_color_depth == 8) {
			color_depth = 4;
			output_select = HDMI_VP_CONF_OUTPUT_SELECTOR_BYPASS;
		} else if (hdmi_data->enc_color_depth == 10) {
			color_depth = 5;
		} else if (hdmi_data->enc_color_depth == 12) {
			color_depth = 6;
		} else if (hdmi_data->enc_color_depth == 16) {
			color_depth = 7;
		} else {
			return;
		}
	} else if (hdmi_data->enc_out_format == YCBCR422_8BITS) {
		if (!hdmi_data->enc_color_depth ||
		    hdmi_data->enc_color_depth == 8)
			remap_size = HDMI_VP_REMAP_YCC422_16bit;
		else if (hdmi_data->enc_color_depth == 10)
			remap_size = HDMI_VP_REMAP_YCC422_20bit;
		else if (hdmi_data->enc_color_depth == 12)
			remap_size = HDMI_VP_REMAP_YCC422_24bit;
		else
			return;
		output_select = HDMI_VP_CONF_OUTPUT_SELECTOR_YCC422;
	} else {
		return;
	}

	/* set the packetizer registers */
	val = ((color_depth << HDMI_VP_PR_CD_COLOR_DEPTH_OFFSET) &
		HDMI_VP_PR_CD_COLOR_DEPTH_MASK) |
		((hdmi_data->pix_repet_factor <<
		HDMI_VP_PR_CD_DESIRED_PR_FACTOR_OFFSET) &
		HDMI_VP_PR_CD_DESIRED_PR_FACTOR_MASK);
	hdmi_writeb(hdmi, val, HDMI_VP_PR_CD);

	hdmi_modb(hdmi, HDMI_VP_STUFF_PR_STUFFING_STUFFING_MODE,
		  HDMI_VP_STUFF_PR_STUFFING_MASK, HDMI_VP_STUFF);

	/* Data from pixel repeater block */
	if (hdmi_data->pix_repet_factor > 1) {
		vp_conf = HDMI_VP_CONF_PR_EN_ENABLE |
			  HDMI_VP_CONF_BYPASS_SELECT_PIX_REPEATER;
	} else { /* data from packetizer block */
		vp_conf = HDMI_VP_CONF_PR_EN_DISABLE |
			  HDMI_VP_CONF_BYPASS_SELECT_VID_PACKETIZER;
	}

	hdmi_modb(hdmi, vp_conf,
		  HDMI_VP_CONF_PR_EN_MASK |
		  HDMI_VP_CONF_BYPASS_SELECT_MASK, HDMI_VP_CONF);

	hdmi_modb(hdmi, 1 << HDMI_VP_STUFF_IDEFAULT_PHASE_OFFSET,
		  HDMI_VP_STUFF_IDEFAULT_PHASE_MASK, HDMI_VP_STUFF);

	hdmi_writeb(hdmi, remap_size, HDMI_VP_REMAP);

	if (output_select == HDMI_VP_CONF_OUTPUT_SELECTOR_PP) {
		vp_conf = HDMI_VP_CONF_BYPASS_EN_DISABLE |
			  HDMI_VP_CONF_PP_EN_ENABLE |
			  HDMI_VP_CONF_YCC422_EN_DISABLE;
	} else if (output_select == HDMI_VP_CONF_OUTPUT_SELECTOR_YCC422) {
		vp_conf = HDMI_VP_CONF_BYPASS_EN_DISABLE |
			  HDMI_VP_CONF_PP_EN_DISABLE |
			  HDMI_VP_CONF_YCC422_EN_ENABLE;
	} else if (output_select == HDMI_VP_CONF_OUTPUT_SELECTOR_BYPASS) {
		vp_conf = HDMI_VP_CONF_BYPASS_EN_ENABLE |
			  HDMI_VP_CONF_PP_EN_DISABLE |
			  HDMI_VP_CONF_YCC422_EN_DISABLE;
	} else {
		return;
	}

	hdmi_modb(hdmi, vp_conf,
		  HDMI_VP_CONF_BYPASS_EN_MASK | HDMI_VP_CONF_PP_EN_ENMASK |
		  HDMI_VP_CONF_YCC422_EN_MASK, HDMI_VP_CONF);

	hdmi_modb(hdmi, HDMI_VP_STUFF_PP_STUFFING_STUFFING_MODE |
			HDMI_VP_STUFF_YCC422_STUFFING_STUFFING_MODE,
		  HDMI_VP_STUFF_PP_STUFFING_MASK |
		  HDMI_VP_STUFF_YCC422_STUFFING_MASK, HDMI_VP_STUFF);

	hdmi_modb(hdmi, output_select, HDMI_VP_CONF_OUTPUT_SELECTOR_MASK,
		  HDMI_VP_CONF);
}

static inline void hdmi_phy_test_clear(struct dw_hdmi *hdmi,
				       unsigned char bit)
{
	hdmi_modb(hdmi, bit << HDMI_PHY_TST0_TSTCLR_OFFSET,
		  HDMI_PHY_TST0_TSTCLR_MASK, HDMI_PHY_TST0);
}

static inline void hdmi_phy_test_enable(struct dw_hdmi *hdmi,
					unsigned char bit)
{
	hdmi_modb(hdmi, bit << HDMI_PHY_TST0_TSTEN_OFFSET,
		  HDMI_PHY_TST0_TSTEN_MASK, HDMI_PHY_TST0);
}

static inline void hdmi_phy_test_clock(struct dw_hdmi *hdmi,
				       unsigned char bit)
{
	hdmi_modb(hdmi, bit << HDMI_PHY_TST0_TSTCLK_OFFSET,
		  HDMI_PHY_TST0_TSTCLK_MASK, HDMI_PHY_TST0);
}

static inline void hdmi_phy_test_din(struct dw_hdmi *hdmi,
				     unsigned char bit)
{
	hdmi_writeb(hdmi, bit, HDMI_PHY_TST1);
}

static inline void hdmi_phy_test_dout(struct dw_hdmi *hdmi,
				      unsigned char bit)
{
	hdmi_writeb(hdmi, bit, HDMI_PHY_TST2);
}

static bool hdmi_phy_wait_i2c_done(struct dw_hdmi *hdmi, int msec)
{
	u32 val;

	while ((val = hdmi_readb(hdmi, HDMI_IH_I2CMPHY_STAT0) & 0x3) == 0) {
		if (msec-- == 0)
			return false;
		udelay(1000);
	}
	hdmi_writeb(hdmi, val, HDMI_IH_I2CMPHY_STAT0);

	return true;
}

static void __hdmi_phy_i2c_write(struct dw_hdmi *hdmi, unsigned short data,
				 unsigned char addr)
{
	hdmi_writeb(hdmi, 0xFF, HDMI_IH_I2CMPHY_STAT0);
	hdmi_writeb(hdmi, addr, HDMI_PHY_I2CM_ADDRESS_ADDR);
	hdmi_writeb(hdmi, (unsigned char)(data >> 8),
		    HDMI_PHY_I2CM_DATAO_1_ADDR);
	hdmi_writeb(hdmi, (unsigned char)(data >> 0),
		    HDMI_PHY_I2CM_DATAO_0_ADDR);
	hdmi_writeb(hdmi, HDMI_PHY_I2CM_OPERATION_ADDR_WRITE,
		    HDMI_PHY_I2CM_OPERATION_ADDR);
	hdmi_phy_wait_i2c_done(hdmi, 1000);
}

static int hdmi_phy_i2c_write(struct dw_hdmi *hdmi, unsigned short data,
			      unsigned char addr)
{
	__hdmi_phy_i2c_write(hdmi, data, addr);
	return 0;
}

static void dw_hdmi_phy_enable_power(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_PDZ_OFFSET,
			 HDMI_PHY_CONF0_PDZ_MASK);
}

static void dw_hdmi_phy_enable_tmds(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_ENTMDS_OFFSET,
			 HDMI_PHY_CONF0_ENTMDS_MASK);
}

static void dw_hdmi_phy_gen2_pddq(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_GEN2_PDDQ_OFFSET,
			 HDMI_PHY_CONF0_GEN2_PDDQ_MASK);
}

static void dw_hdmi_phy_gen2_txpwron(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_GEN2_TXPWRON_OFFSET,
			 HDMI_PHY_CONF0_GEN2_TXPWRON_MASK);
}

static void dw_hdmi_phy_sel_data_en_pol(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_SELDATAENPOL_OFFSET,
			 HDMI_PHY_CONF0_SELDATAENPOL_MASK);
}

static void dw_hdmi_phy_sel_interface_control(struct dw_hdmi *hdmi, u8 enable)
{
	hdmi_mask_writeb(hdmi, enable, HDMI_PHY_CONF0,
			 HDMI_PHY_CONF0_SELDIPIF_OFFSET,
			 HDMI_PHY_CONF0_SELDIPIF_MASK);
}

enum {
	DW_HDMI_RES_8,
	DW_HDMI_RES_10,
	DW_HDMI_RES_12,
	DW_HDMI_RES_MAX,
};

struct mpll_config {
	unsigned long mpixelclock;
	struct {
		u16 cpce;
		u16 gmp;
	} res[DW_HDMI_RES_MAX];
};

static const struct mpll_config mpll_config[] = {
	{
		45250000, {
			{ 0x01e0, 0x0000 },
			{ 0x21e1, 0x0000 },
			{ 0x41e2, 0x0000 }
		},
	}, {
		92500000, {
			{ 0x0140, 0x0005 },
			{ 0x2141, 0x0005 },
			{ 0x4142, 0x0005 },
		},
	}, {
		148500000, {
			{ 0x00a0, 0x000a },
			{ 0x20a1, 0x000a },
			{ 0x40a2, 0x000a },
		},
	}, {
		~0UL, {
			{ 0x00a0, 0x000a },
			{ 0x2001, 0x000f },
			{ 0x4002, 0x000f },
		},
	}
};

struct curr_ctrl {
	unsigned long mpixelclock;
	u16 curr[DW_HDMI_RES_MAX];
};

static const struct curr_ctrl curr_ctrl[] = {
	/*	pixelclk     bpp8    bpp10   bpp12 */
	{
		 54000000, { 0x091c, 0x091c, 0x06dc },
	}, {
		 58400000, { 0x091c, 0x06dc, 0x06dc },
	}, {
		 72000000, { 0x06dc, 0x06dc, 0x091c },
	}, {
		 74250000, { 0x06dc, 0x0b5c, 0x091c },
	}, {
		118800000, { 0x091c, 0x091c, 0x06dc },
	}, {
		216000000, { 0x06dc, 0x0b5c, 0x091c },
	}
};

static int hdmi_phy_configure(struct dw_hdmi *hdmi, unsigned char prep,
			      unsigned char res, int cscon)
{
	unsigned res_idx, i;
	u8 val, msec;

	if (prep)
		return -EINVAL;

	switch (res) {
	case 0:	/* color resolution 0 is 8 bit colour depth */
	case 8:
		res_idx = DW_HDMI_RES_8;
		break;
	case 10:
		res_idx = DW_HDMI_RES_10;
		break;
	case 12:
		res_idx = DW_HDMI_RES_12;
		break;
	default:
		return -EINVAL;
	}

	/* Enable csc path */
	if (cscon)
		val = HDMI_MC_FLOWCTRL_FEED_THROUGH_OFF_CSC_IN_PATH;
	else
		val = HDMI_MC_FLOWCTRL_FEED_THROUGH_OFF_CSC_BYPASS;

	hdmi_writeb(hdmi, val, HDMI_MC_FLOWCTRL);

	/* gen2 tx power off */
	dw_hdmi_phy_gen2_txpwron(hdmi, 0);

	/* gen2 pddq */
	dw_hdmi_phy_gen2_pddq(hdmi, 1);

	/* PHY reset */
	hdmi_writeb(hdmi, HDMI_MC_PHYRSTZ_DEASSERT, HDMI_MC_PHYRSTZ);
	hdmi_writeb(hdmi, HDMI_MC_PHYRSTZ_ASSERT, HDMI_MC_PHYRSTZ);

	hdmi_writeb(hdmi, HDMI_MC_HEACPHY_RST_ASSERT, HDMI_MC_HEACPHY_RST);

	hdmi_phy_test_clear(hdmi, 1);
	hdmi_writeb(hdmi, HDMI_PHY_I2CM_SLAVE_ADDR_PHY_GEN2,
		    HDMI_PHY_I2CM_SLAVE_ADDR);
	hdmi_phy_test_clear(hdmi, 0);

	/* PLL/MPLL Cfg - always match on final entry */
	for (i = 0; i < ARRAY_SIZE(mpll_config) - 1; i++)
		if (hdmi->hdmi_data.video_mode.mpixelclock <=
		    mpll_config[i].mpixelclock)
			break;

	hdmi_phy_i2c_write(hdmi, mpll_config[i].res[res_idx].cpce, 0x06);
	hdmi_phy_i2c_write(hdmi, mpll_config[i].res[res_idx].gmp, 0x15);

	for (i = 0; i < ARRAY_SIZE(curr_ctrl); i++)
		if (hdmi->hdmi_data.video_mode.mpixelclock <=
		    curr_ctrl[i].mpixelclock)
			break;

	if (i >= ARRAY_SIZE(curr_ctrl)) {
		dev_err(hdmi->dev,
				"Pixel clock %d - unsupported by HDMI\n",
				hdmi->hdmi_data.video_mode.mpixelclock);
		return -EINVAL;
	}

	/* CURRCTRL */
	hdmi_phy_i2c_write(hdmi, curr_ctrl[i].curr[res_idx], 0x10);

	hdmi_phy_i2c_write(hdmi, 0x0000, 0x13);  /* PLLPHBYCTRL */
	hdmi_phy_i2c_write(hdmi, 0x0006, 0x17);
	/* RESISTANCE TERM 133Ohm Cfg */
	hdmi_phy_i2c_write(hdmi, 0x0005, 0x19);  /* TXTERM */
	/* PREEMP Cgf 0.00 */
	hdmi_phy_i2c_write(hdmi, 0x800d, 0x09);  /* CKSYMTXCTRL */
	/* TX/CK LVL 10 */
	hdmi_phy_i2c_write(hdmi, 0x01ad, 0x0E);  /* VLEVCTRL */
	/* REMOVE CLK TERM */
	hdmi_phy_i2c_write(hdmi, 0x8000, 0x05);  /* CKCALCTRL */

	dw_hdmi_phy_enable_power(hdmi, 1);

	/* toggle TMDS enable */
	dw_hdmi_phy_enable_tmds(hdmi, 0);
	dw_hdmi_phy_enable_tmds(hdmi, 1);

	/* gen2 tx power on */
	dw_hdmi_phy_gen2_txpwron(hdmi, 1);
	dw_hdmi_phy_gen2_pddq(hdmi, 0);

	/*Wait for PHY PLL lock */
	msec = 5;
	do {
		val = hdmi_readb(hdmi, HDMI_PHY_STAT0) & HDMI_PHY_TX_PHY_LOCK;
		if (!val)
			break;

		if (msec == 0) {
			dev_err(hdmi->dev, "PHY PLL not locked\n");
			return -ETIMEDOUT;
		}

		udelay(1000);
		msec--;
	} while (1);

	return 0;
}

static int dw_hdmi_phy_init(struct dw_hdmi *hdmi)
{
	int i, ret;
	bool cscon = false;

	/*check csc whether needed activated in HDMI mode */
	cscon = (is_color_space_conversion(hdmi) &&
			!hdmi->hdmi_data.video_mode.mdvi);

	/* HDMI Phy spec says to do the phy initialization sequence twice */
	for (i = 0; i < 2; i++) {
		dw_hdmi_phy_sel_data_en_pol(hdmi, 1);
		dw_hdmi_phy_sel_interface_control(hdmi, 0);
		dw_hdmi_phy_enable_tmds(hdmi, 0);
		dw_hdmi_phy_enable_power(hdmi, 0);

		/* Enable CSC */
		ret = hdmi_phy_configure(hdmi, 0, 8, cscon);
		if (ret)
			return ret;
	}

	hdmi->phy_enabled = true;
	return 0;
}

static void hdmi_tx_hdcp_config(struct dw_hdmi *hdmi)
{
	u8 de;

	if (hdmi->hdmi_data.video_mode.mdataenablepolarity)
		de = HDMI_A_VIDPOLCFG_DATAENPOL_ACTIVE_HIGH;
	else
		de = HDMI_A_VIDPOLCFG_DATAENPOL_ACTIVE_LOW;

	/* disable rx detect */
	hdmi_modb(hdmi, HDMI_A_HDCPCFG0_RXDETECT_DISABLE,
		  HDMI_A_HDCPCFG0_RXDETECT_MASK, HDMI_A_HDCPCFG0);

	hdmi_modb(hdmi, de, HDMI_A_VIDPOLCFG_DATAENPOL_MASK, HDMI_A_VIDPOLCFG);

	hdmi_modb(hdmi, HDMI_A_HDCPCFG1_ENCRYPTIONDISABLE_DISABLE,
		  HDMI_A_HDCPCFG1_ENCRYPTIONDISABLE_MASK, HDMI_A_HDCPCFG1);
}

static void hdmi_av_composer(struct dw_hdmi *hdmi)
{
	u8 inv_val;
	struct hdmi_vmode *vmode = &hdmi->hdmi_data.video_mode;
	int hblank, vblank;
	struct fb_videomode *mode = hdmi->mode;

	vmode->mhsyncpolarity = !!(mode->sync & FB_SYNC_HOR_HIGH_ACT);
	vmode->mvsyncpolarity = !!(mode->sync & FB_SYNC_VERT_HIGH_ACT);
	vmode->minterlaced = !!(mode->vmode & FB_VMODE_INTERLACED);
	vmode->mpixelclock = PICOS2KHZ(mode->pixclock) * 1000;

	dev_dbg(hdmi->dev, "final pixclk = %d\n", vmode->mpixelclock);

	/* Set up HDMI_FC_INVIDCONF */
	inv_val = (hdmi->hdmi_data.hdcp_enable ?
		HDMI_FC_INVIDCONF_HDCP_KEEPOUT_ACTIVE :
		HDMI_FC_INVIDCONF_HDCP_KEEPOUT_INACTIVE);

	inv_val |= (vmode->mvsyncpolarity ?
		HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_HIGH :
		HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_LOW);

	inv_val |= (vmode->mhsyncpolarity ?
		HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_HIGH :
		HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_LOW);

	inv_val |= (vmode->mdataenablepolarity ?
		HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH :
		HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_LOW);

	if (hdmi->vic == 39)
		inv_val |= HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_HIGH;
	else
		inv_val |= (vmode->minterlaced ?
			HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_HIGH :
			HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_LOW);

	inv_val |= (vmode->minterlaced ?
		HDMI_FC_INVIDCONF_IN_I_P_INTERLACED :
		HDMI_FC_INVIDCONF_IN_I_P_PROGRESSIVE);

	inv_val |= (vmode->mdvi ?
		HDMI_FC_INVIDCONF_DVI_MODEZ_DVI_MODE :
		HDMI_FC_INVIDCONF_DVI_MODEZ_HDMI_MODE);

	hdmi_writeb(hdmi, inv_val, HDMI_FC_INVIDCONF);

	/* Set up horizontal active pixel width */
	hdmi_writeb(hdmi, mode->xres >> 8, HDMI_FC_INHACTV1);
	hdmi_writeb(hdmi, mode->xres, HDMI_FC_INHACTV0);

	/* Set up vertical active lines */
	hdmi_writeb(hdmi, mode->yres >> 8, HDMI_FC_INVACTV1);
	hdmi_writeb(hdmi, mode->yres, HDMI_FC_INVACTV0);

	/* Set up horizontal blanking pixel region width */
	hblank = mode->left_margin + mode->right_margin + mode->hsync_len;
	hdmi_writeb(hdmi, hblank >> 8, HDMI_FC_INHBLANK1);
	hdmi_writeb(hdmi, hblank, HDMI_FC_INHBLANK0);

	/* Set up vertical blanking pixel region width */
	vblank = mode->upper_margin + mode->lower_margin + mode->vsync_len;
	hdmi_writeb(hdmi, vblank, HDMI_FC_INVBLANK);

	/* Set up HSYNC active edge delay width (in pixel clks) */
	hdmi_writeb(hdmi, mode->right_margin >> 8, HDMI_FC_HSYNCINDELAY1);
	hdmi_writeb(hdmi, mode->right_margin, HDMI_FC_HSYNCINDELAY0);

	/* Set up VSYNC active edge delay (in lines) */
	hdmi_writeb(hdmi, mode->lower_margin, HDMI_FC_VSYNCINDELAY);

	/* Set up HSYNC active pulse width (in pixel clks) */
	hdmi_writeb(hdmi, mode->hsync_len >> 8, HDMI_FC_HSYNCINWIDTH1);
	hdmi_writeb(hdmi, mode->hsync_len, HDMI_FC_HSYNCINWIDTH0);

	/* Set up VSYNC active edge delay (in lines) */
	hdmi_writeb(hdmi, mode->vsync_len, HDMI_FC_VSYNCINWIDTH);
}

static void dw_hdmi_phy_disable(struct dw_hdmi *hdmi)
{
	if (!hdmi->phy_enabled)
		return;

	dw_hdmi_phy_enable_tmds(hdmi, 0);
	dw_hdmi_phy_enable_power(hdmi, 0);

	hdmi->phy_enabled = false;
}

/* HDMI Initialization Step B.4 */
static void dw_hdmi_enable_video_path(struct dw_hdmi *hdmi)
{
	u8 clkdis;

	/* control period minimum duration */
	hdmi_writeb(hdmi, 12, HDMI_FC_CTRLDUR);
	hdmi_writeb(hdmi, 32, HDMI_FC_EXCTRLDUR);
	hdmi_writeb(hdmi, 1, HDMI_FC_EXCTRLSPAC);

	/* Set to fill TMDS data channels */
	hdmi_writeb(hdmi, 0x0B, HDMI_FC_CH0PREAM);
	hdmi_writeb(hdmi, 0x16, HDMI_FC_CH1PREAM);
	hdmi_writeb(hdmi, 0x21, HDMI_FC_CH2PREAM);

	/* Enable pixel clock and tmds data path */
	clkdis = 0x7F;
	clkdis &= ~HDMI_MC_CLKDIS_PIXELCLK_DISABLE;
	hdmi_writeb(hdmi, clkdis, HDMI_MC_CLKDIS);

	clkdis &= ~HDMI_MC_CLKDIS_TMDSCLK_DISABLE;
	hdmi_writeb(hdmi, clkdis, HDMI_MC_CLKDIS);

	/* Enable csc path */
	if (is_color_space_conversion(hdmi)) {
		clkdis &= ~HDMI_MC_CLKDIS_CSCCLK_DISABLE;
		hdmi_writeb(hdmi, clkdis, HDMI_MC_CLKDIS);
	}
}

/* Workaround to clear the overflow condition */
static void dw_hdmi_clear_overflow(struct dw_hdmi *hdmi)
{
	int count;
	u8 val;

	/* TMDS software reset */
	hdmi_writeb(hdmi, (u8)~HDMI_MC_SWRSTZ_TMDSSWRST_REQ, HDMI_MC_SWRSTZ);

	val = hdmi_readb(hdmi, HDMI_FC_INVIDCONF);
	if (hdmi->dev_type == IMX6DL_HDMI) {
		hdmi_writeb(hdmi, val, HDMI_FC_INVIDCONF);
		return;
	}

	for (count = 0; count < 4; count++)
		hdmi_writeb(hdmi, val, HDMI_FC_INVIDCONF);
}

static int dw_hdmi_setup(struct dw_hdmi *hdmi)
{
	int ret;

	dev_dbg(hdmi->dev, "Non-CEA mode used in HDMI\n");
	hdmi->hdmi_data.video_mode.mdvi = true;

	hdmi->hdmi_data.colorimetry = HDMI_COLORIMETRY_ITU_709;
	hdmi->hdmi_data.video_mode.mpixelrepetitionoutput = 0;
	hdmi->hdmi_data.video_mode.mpixelrepetitioninput = 0;

	/* TODO: Get input format from IPU (via FB driver interface) */
	hdmi->hdmi_data.enc_in_format = RGB;

	hdmi->hdmi_data.enc_out_format = RGB;

	hdmi->hdmi_data.enc_color_depth = 8;
	hdmi->hdmi_data.pix_repet_factor = 0;
	hdmi->hdmi_data.hdcp_enable = 0;
	hdmi->hdmi_data.video_mode.mdataenablepolarity = true;

	/* HDMI Initialization Step B.1 */
	hdmi_av_composer(hdmi);

	/* HDMI Initializateion Step B.2 */
	ret = dw_hdmi_phy_init(hdmi);
	if (ret)
		return ret;

	/* HDMI Initialization Step B.3 */
	dw_hdmi_enable_video_path(hdmi);

	hdmi_video_packetize(hdmi);
	hdmi_video_csc(hdmi);
	hdmi_video_sample(hdmi);
	hdmi_tx_hdcp_config(hdmi);

	dw_hdmi_clear_overflow(hdmi);

	return 0;
}

static void initialize_hdmi_ih_mutes(struct dw_hdmi *hdmi)
{
	u8 ih_mute;

	/*
	 * Boot up defaults are:
	 * HDMI_IH_MUTE   = 0x03 (disabled)
	 * HDMI_IH_MUTE_* = 0x00 (enabled)
	 *
	 * Disable top level interrupt bits in HDMI block
	 */
	ih_mute = hdmi_readb(hdmi, HDMI_IH_MUTE) |
		  HDMI_IH_MUTE_MUTE_WAKEUP_INTERRUPT |
		  HDMI_IH_MUTE_MUTE_ALL_INTERRUPT;

	hdmi_writeb(hdmi, ih_mute, HDMI_IH_MUTE);

	/* by default mask all interrupts */
	hdmi_writeb(hdmi, 0xff, HDMI_VP_MASK);
	hdmi_writeb(hdmi, 0xff, HDMI_FC_MASK0);
	hdmi_writeb(hdmi, 0xff, HDMI_FC_MASK1);
	hdmi_writeb(hdmi, 0xff, HDMI_FC_MASK2);
	hdmi_writeb(hdmi, 0xff, HDMI_PHY_MASK0);
	hdmi_writeb(hdmi, 0xff, HDMI_PHY_I2CM_INT_ADDR);
	hdmi_writeb(hdmi, 0xff, HDMI_PHY_I2CM_CTLINT_ADDR);
	hdmi_writeb(hdmi, 0xff, HDMI_AUD_INT);
	hdmi_writeb(hdmi, 0xff, HDMI_AUD_SPDIFINT);
	hdmi_writeb(hdmi, 0xff, HDMI_AUD_HBR_MASK);
	hdmi_writeb(hdmi, 0xff, HDMI_GP_MASK);
	hdmi_writeb(hdmi, 0xff, HDMI_A_APIINTMSK);
	hdmi_writeb(hdmi, 0xff, HDMI_CEC_MASK);
	hdmi_writeb(hdmi, 0xff, HDMI_I2CM_INT);
	hdmi_writeb(hdmi, 0xff, HDMI_I2CM_CTLINT);

	/* Disable interrupts in the IH_MUTE_* registers */
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_FC_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_FC_STAT1);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_FC_STAT2);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_AS_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_PHY_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_I2CM_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_CEC_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_VP_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_I2CMPHY_STAT0);
	hdmi_writeb(hdmi, 0xff, HDMI_IH_MUTE_AHBDMAAUD_STAT0);

	/* Enable top level interrupt bits in HDMI block */
	ih_mute &= ~(HDMI_IH_MUTE_MUTE_WAKEUP_INTERRUPT |
		    HDMI_IH_MUTE_MUTE_ALL_INTERRUPT);
	hdmi_writeb(hdmi, ih_mute, HDMI_IH_MUTE);
}

struct dw_hdmi_data {
	unsigned ipu_mask;
	enum dw_hdmi_devtype devtype;
};

static struct dw_hdmi_data imx6q_hdmi_data = {
	.ipu_mask = 0xf,
	.devtype = IMX6Q_HDMI,
};

static struct dw_hdmi_data imx6dl_hdmi_data = {
	.ipu_mask = 0x3,
	.devtype = IMX6DL_HDMI,
};

static struct of_device_id dw_hdmi_dt_ids[] = {
	{
		.compatible = "fsl,imx6q-hdmi",
		.data = &imx6q_hdmi_data,
	}, {
		.compatible = "fsl,imx6dl-hdmi",
		.data = &imx6dl_hdmi_data,
	}, {
		/* sentinel */
	}
};

static int dw_hdmi_get_modes(struct dw_hdmi *hdmi, struct display_timings *timings)
{
	int ret = -ENOENT;

	if (hdmi->ddc_node) {
		struct i2c_adapter *i2c;

                i2c = of_find_i2c_adapter_by_node(hdmi->ddc_node);
		if (!i2c)
			return -ENODEV;
		timings->edid = edid_read_i2c(i2c);
		if (!timings->edid)
			return -EINVAL;

		ret = edid_to_display_timings(timings, timings->edid);
	}

	return ret;
}

static int dw_hdmi_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *data)
{
	struct dw_hdmi *hdmi = container_of(vpl, struct dw_hdmi, vpl);
	struct ipu_di_mode *mode;

	switch (cmd) {
	case VPL_ENABLE:
		return dw_hdmi_setup(hdmi);
	case VPL_DISABLE:
		dw_hdmi_phy_disable(hdmi);
		return 0;
	case VPL_PREPARE:
		hdmi->mode = data;
		dw_hdmi_set_ipu_di_mux(hdmi, port);
		return 0;
	case VPL_GET_VIDEOMODES:
		return dw_hdmi_get_modes(hdmi, data);
	case IMX_IPU_VPL_DI_MODE:
		mode = data;

		mode->di_clkflags = IPU_DI_CLKMODE_EXT | IPU_DI_CLKMODE_SYNC;
		mode->bus_format = MEDIA_BUS_FMT_RGB888_1X24;

		return 0;
	}

	return 0;
}

static int dw_hdmi_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	struct dw_hdmi *hdmi;
	int ret;
	const struct dw_hdmi_data *devtype;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	hdmi = xzalloc(sizeof(*hdmi));

	hdmi->dev = dev;
	hdmi->connected = 0;
	hdmi->sample_rate = 48000;
	hdmi->ratio = 100;

	ret = dev_get_drvdata(dev, (const void **)&hdmi->dev_type);
	if (ret)
		return ret;

	hdmi->ddc_node = of_parse_phandle(np, "ddc-i2c-bus", 0);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	hdmi->regs = IOMEM(iores->start);

	hdmi->isfr_clk = clk_get(hdmi->dev, "isfr");
	if (IS_ERR(hdmi->isfr_clk)) {
		ret = PTR_ERR(hdmi->isfr_clk);
		dev_err(dev,
			"Unable to get HDMI isfr clk: %d\n", ret);
		return ret;
	}

	ret = clk_enable(hdmi->isfr_clk);
	if (ret)
		return ret;

	hdmi->iahb_clk = clk_get(hdmi->dev, "iahb");
	if (IS_ERR(hdmi->iahb_clk)) {
		ret = PTR_ERR(hdmi->iahb_clk);
		dev_err(dev,
			"Unable to get HDMI iahb clk: %d\n", ret);
		goto err_isfr;
	}

	ret = clk_enable(hdmi->iahb_clk);
	if (ret)
		goto err_isfr;

	/* Product and revision IDs */
	dev_info(dev,
		"Detected HDMI controller 0x%x:0x%x:0x%x:0x%x\n",
		hdmi_readb(hdmi, HDMI_DESIGN_ID),
		hdmi_readb(hdmi, HDMI_REVISION_ID),
		hdmi_readb(hdmi, HDMI_PRODUCT_ID0),
		hdmi_readb(hdmi, HDMI_PRODUCT_ID1));

	initialize_hdmi_ih_mutes(hdmi);

	/*
	 * To prevent overflows in HDMI_IH_FC_STAT2, set the clk regenerator
	 * N and cts values before enabling phy
	 */
	hdmi_init_clk_regenerator(hdmi);

	/*
	 * Configure registers related to HDMI interrupt
	 * generation before registering IRQ.
	 */
	hdmi_writeb(hdmi, HDMI_PHY_HPD, HDMI_PHY_POL0);

	/* Clear Hotplug interrupts */
	hdmi_writeb(hdmi, HDMI_IH_PHY_STAT0_HPD, HDMI_IH_PHY_STAT0);

	hdmi_writeb(hdmi, HDMI_PHY_I2CM_INT_ADDR_DONE_POL,
		    HDMI_PHY_I2CM_INT_ADDR);

	hdmi_writeb(hdmi, HDMI_PHY_I2CM_CTLINT_ADDR_NAC_POL |
		    HDMI_PHY_I2CM_CTLINT_ADDR_ARBITRATION_POL,
		    HDMI_PHY_I2CM_CTLINT_ADDR);

	/* enable cable hot plug irq */
	hdmi_writeb(hdmi, (u8)~HDMI_PHY_HPD, HDMI_PHY_MASK0);

	/* Unmute interrupts */
	hdmi_writeb(hdmi, ~HDMI_IH_PHY_STAT0_HPD, HDMI_IH_MUTE_PHY_STAT0);

	hdmi->vpl.node = np;
	hdmi->vpl.ioctl = dw_hdmi_ioctl;
	ret = vpl_register(&hdmi->vpl);
	if (ret)
		return ret;

	return 0;

err_isfr:
	clk_disable(hdmi->isfr_clk);

	return ret;
}

static struct driver_d dw_hdmi_driver = {
	.probe		= dw_hdmi_probe,
	.of_compatible	= dw_hdmi_dt_ids,
	.name		= "imx-hdmi",
};
device_platform_driver(dw_hdmi_driver);

MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
MODULE_DESCRIPTION("i.MX6 HDMI transmitter driver");
MODULE_LICENSE("GPL");
