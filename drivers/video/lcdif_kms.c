// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Marek Vasut <marex@denx.de>
 *
 * This code is based on drivers/gpu/drm/mxsfb/mxsfb*
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <video/media-bus-format.h>

#include <video/drm/drm_connector.h>
#include <of_graph.h>
#include <fb.h>
#include <dma.h>

#include <video/vpl.h>
#include <video/videomode.h>

#include <video/fourcc.h>

#include "lcdif_drv.h"
#include "lcdif_regs.h"

struct lcdif_crtc_state {
	u32			bus_format;
	u32			bus_flags;
};

static void lcdif_set_formats(struct lcdif_drm_private *lcdif,
			      const u32 format,
			      const u32 bus_format)
{
	bool in_yuv = false;
	bool out_yuv = false;

	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB565_1X16:
		writel(DISP_PARA_LINE_PATTERN_RGB565,
		       lcdif->base + LCDC_V8_DISP_PARA);
		break;
	case MEDIA_BUS_FMT_BGR888_1X24:
		writel(DISP_PARA_LINE_PATTERN_BGR888,
		       lcdif->base + LCDC_V8_DISP_PARA);
		break;
	case MEDIA_BUS_FMT_RGB888_1X24:
		writel(DISP_PARA_LINE_PATTERN_RGB888,
		       lcdif->base + LCDC_V8_DISP_PARA);
		break;
	case MEDIA_BUS_FMT_UYVY8_1X16:
		writel(DISP_PARA_LINE_PATTERN_UYVY_H,
		       lcdif->base + LCDC_V8_DISP_PARA);
		out_yuv = true;
		break;
	default:
		dev_err(lcdif->dev, "Unknown media bus format 0x%x\n", bus_format);
		break;
	}

	switch (format) {
	/* RGB Formats */
	case DRM_FORMAT_RGB565:
		writel(CTRLDESCL0_5_BPP_16_RGB565,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;
	case DRM_FORMAT_RGB888:
		writel(CTRLDESCL0_5_BPP_24_RGB888,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;
	case DRM_FORMAT_XRGB1555:
		writel(CTRLDESCL0_5_BPP_16_ARGB1555,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;
	case DRM_FORMAT_XRGB4444:
		writel(CTRLDESCL0_5_BPP_16_ARGB4444,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;
	case DRM_FORMAT_XBGR8888:
		writel(CTRLDESCL0_5_BPP_32_ABGR8888,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;
	case DRM_FORMAT_XRGB8888:
		writel(CTRLDESCL0_5_BPP_32_ARGB8888,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		break;

	/* YUV Formats */
	case DRM_FORMAT_YUYV:
		writel(CTRLDESCL0_5_BPP_YCbCr422 | CTRLDESCL0_5_YUV_FORMAT_VY2UY1,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		in_yuv = true;
		break;
	case DRM_FORMAT_YVYU:
		writel(CTRLDESCL0_5_BPP_YCbCr422 | CTRLDESCL0_5_YUV_FORMAT_UY2VY1,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		in_yuv = true;
		break;
	case DRM_FORMAT_UYVY:
		writel(CTRLDESCL0_5_BPP_YCbCr422 | CTRLDESCL0_5_YUV_FORMAT_Y2VY1U,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		in_yuv = true;
		break;
	case DRM_FORMAT_VYUY:
		writel(CTRLDESCL0_5_BPP_YCbCr422 | CTRLDESCL0_5_YUV_FORMAT_Y2UY1V,
		       lcdif->base + LCDC_V8_CTRLDESCL0_5);
		in_yuv = true;
		break;

	default:
		dev_err(lcdif->dev, "Unknown pixel format 0x%x\n", format);
		break;
	}

	/*
	 * The CSC differentiates between "YCbCr" and "YUV", but the reference
	 * manual doesn't detail how they differ. Experiments showed that the
	 * luminance value is unaffected, only the calculations involving chroma
	 * values differ. The YCbCr mode behaves as expected, with chroma values
	 * being offset by 128. The YUV mode isn't fully understood.
	 */
	if (!in_yuv && out_yuv) {
		/* RGB -> YCbCr */
		writel(CSC0_CTRL_CSC_MODE_RGB2YCbCr,
		       lcdif->base + LCDC_V8_CSC0_CTRL);

		/*
		 * CSC: BT.601 Limited Range RGB to YCbCr coefficients.
		 *
		 * |Y |   | 0.2568  0.5041  0.0979|   |R|   |16 |
		 * |Cb| = |-0.1482 -0.2910  0.4392| * |G| + |128|
		 * |Cr|   | 0.4392  0.4392 -0.3678|   |B|   |128|
		 */
		writel(CSC0_COEF0_A2(0x081) | CSC0_COEF0_A1(0x041),
		       lcdif->base + LCDC_V8_CSC0_COEF0);
		writel(CSC0_COEF1_B1(0x7db) | CSC0_COEF1_A3(0x019),
		       lcdif->base + LCDC_V8_CSC0_COEF1);
		writel(CSC0_COEF2_B3(0x070) | CSC0_COEF2_B2(0x7b6),
		       lcdif->base + LCDC_V8_CSC0_COEF2);
		writel(CSC0_COEF3_C2(0x7a2) | CSC0_COEF3_C1(0x070),
		       lcdif->base + LCDC_V8_CSC0_COEF3);
		writel(CSC0_COEF4_D1(0x010) | CSC0_COEF4_C3(0x7ee),
		       lcdif->base + LCDC_V8_CSC0_COEF4);
		writel(CSC0_COEF5_D3(0x080) | CSC0_COEF5_D2(0x080),
		       lcdif->base + LCDC_V8_CSC0_COEF5);
	} else if (in_yuv && !out_yuv) {
		/* YCbCr -> RGB */
		/*
		 * BT.601 limited range:
		 *
		 * |R|	 |1.1644  0.0000  1.5960|   |Y	- 16 |
		 * |G| = |1.1644 -0.3917 -0.8129| * |Cb - 128|
		 * |B|	 |1.1644  2.0172  0.0000|   |Cr - 128|
		 */
		const u32 coeffs[6] = {
				CSC0_COEF0_A1(0x12a) | CSC0_COEF0_A2(0x000),
				CSC0_COEF1_A3(0x199) | CSC0_COEF1_B1(0x12a),
				CSC0_COEF2_B2(0x79c) | CSC0_COEF2_B3(0x730),
				CSC0_COEF3_C1(0x12a) | CSC0_COEF3_C2(0x204),
				CSC0_COEF4_C3(0x000) | CSC0_COEF4_D1(0x1f0),
				CSC0_COEF5_D2(0x180) | CSC0_COEF5_D3(0x180),
			};

		writel(CSC0_CTRL_CSC_MODE_YCbCr2RGB,
		       lcdif->base + LCDC_V8_CSC0_CTRL);

		writel(coeffs[0], lcdif->base + LCDC_V8_CSC0_COEF0);
		writel(coeffs[1], lcdif->base + LCDC_V8_CSC0_COEF1);
		writel(coeffs[2], lcdif->base + LCDC_V8_CSC0_COEF2);
		writel(coeffs[3], lcdif->base + LCDC_V8_CSC0_COEF3);
		writel(coeffs[4], lcdif->base + LCDC_V8_CSC0_COEF4);
		writel(coeffs[5], lcdif->base + LCDC_V8_CSC0_COEF5);
	} else {
		/* RGB -> RGB, YCbCr -> YCbCr: bypass colorspace converter. */
		writel(CSC0_CTRL_BYPASS, lcdif->base + LCDC_V8_CSC0_CTRL);
	}
}

static void lcdif_set_mode(struct lcdif_drm_private *lcdif,
			   struct drm_display_mode *m,
			   u32 bus_flags)
{
	u32 ctrl = 0;

	if (m->flags & DRM_MODE_FLAG_NHSYNC)
		ctrl |= CTRL_INV_HS;
	if (m->flags & DRM_MODE_FLAG_NVSYNC)
		ctrl |= CTRL_INV_VS;
	if (bus_flags & DRM_BUS_FLAG_DE_LOW)
		ctrl |= CTRL_INV_DE;
	if (bus_flags & DRM_BUS_FLAG_PIXDATA_DRIVE_NEGEDGE)
		ctrl |= CTRL_INV_PXCK;

	writel(ctrl, lcdif->base + LCDC_V8_CTRL);

	writel(DISP_SIZE_DELTA_Y(m->vdisplay) |
	       DISP_SIZE_DELTA_X(m->hdisplay),
	       lcdif->base + LCDC_V8_DISP_SIZE);

	writel(HSYN_PARA_BP_H(m->htotal - m->hsync_end) |
	       HSYN_PARA_FP_H(m->hsync_start - m->hdisplay),
	       lcdif->base + LCDC_V8_HSYN_PARA);

	writel(VSYN_PARA_BP_V(m->vtotal - m->vsync_end) |
	       VSYN_PARA_FP_V(m->vsync_start - m->vdisplay),
	       lcdif->base + LCDC_V8_VSYN_PARA);

	writel(VSYN_HSYN_WIDTH_PW_V(m->vsync_end - m->vsync_start) |
	       VSYN_HSYN_WIDTH_PW_H(m->hsync_end - m->hsync_start),
	       lcdif->base + LCDC_V8_VSYN_HSYN_WIDTH);

	writel(CTRLDESCL0_1_HEIGHT(m->vdisplay) |
	       CTRLDESCL0_1_WIDTH(m->hdisplay),
	       lcdif->base + LCDC_V8_CTRLDESCL0_1);

	/*
	 * Undocumented P_SIZE and T_SIZE register but those written in the
	 * downstream kernel those registers control the AXI burst size. As of
	 * now there are two known values:
	 *  1 - 128Byte
	 *  2 - 256Byte
	 * Downstream set it to 256B burst size to improve the memory
	 * efficiency so set it here too.
	 */
	/* NOTE: Since this driver is currently fixed to DRM_FORMAT_XRGB8888
	 * we asume a stride of vdisplay * 4
	 */
	ctrl = CTRLDESCL0_3_P_SIZE(2) | CTRLDESCL0_3_T_SIZE(2) |
	       CTRLDESCL0_3_PITCH(m->hdisplay * 4);
	writel(ctrl, lcdif->base + LCDC_V8_CTRLDESCL0_3);
}

static void lcdif_enable_controller(struct lcdif_drm_private *lcdif)
{
	u32 reg;

	/* Set FIFO Panic watermarks, low 1/3, high 2/3 . */
	writel(FIELD_PREP(PANIC0_THRES_LOW_MASK, 1 * PANIC0_THRES_MAX / 3) |
	       FIELD_PREP(PANIC0_THRES_HIGH_MASK, 2 * PANIC0_THRES_MAX / 3),
	       lcdif->base + LCDC_V8_PANIC0_THRES);

	/*
	 * Enable FIFO Panic, this does not generate interrupt, but
	 * boosts NoC priority based on FIFO Panic watermarks.
	 */
	writel(INT_ENABLE_D1_PLANE_PANIC_EN,
	       lcdif->base + LCDC_V8_INT_ENABLE_D1);

	reg = readl(lcdif->base + LCDC_V8_DISP_PARA);
	reg |= DISP_PARA_DISP_ON;
	writel(reg, lcdif->base + LCDC_V8_DISP_PARA);

	reg = readl(lcdif->base + LCDC_V8_CTRLDESCL0_5);
	reg |= CTRLDESCL0_5_EN;
	writel(reg, lcdif->base + LCDC_V8_CTRLDESCL0_5);
}

static void lcdif_disable_controller(struct lcdif_drm_private *lcdif)
{
	u32 reg;
	int ret;

	reg = readl(lcdif->base + LCDC_V8_CTRLDESCL0_5);
	reg &= ~CTRLDESCL0_5_EN;
	writel(reg, lcdif->base + LCDC_V8_CTRLDESCL0_5);

	ret = readl_poll_timeout(lcdif->base + LCDC_V8_CTRLDESCL0_5,
				 reg, !(reg & CTRLDESCL0_5_EN),
				 36000);	/* Wait ~2 frame times max */
	if (ret)
		dev_err(lcdif->dev, "Failed to disable controller!\n");

	reg = readl(lcdif->base + LCDC_V8_DISP_PARA);
	reg &= ~DISP_PARA_DISP_ON;
	writel(reg, lcdif->base + LCDC_V8_DISP_PARA);

	/* Disable FIFO Panic NoC priority booster. */
	writel(0, lcdif->base + LCDC_V8_INT_ENABLE_D1);
}

static void lcdif_reset_block(struct lcdif_drm_private *lcdif)
{
	writel(CTRL_SW_RESET, lcdif->base + LCDC_V8_CTRL + REG_SET);
	readl(lcdif->base + LCDC_V8_CTRL);
	writel(CTRL_SW_RESET, lcdif->base + LCDC_V8_CTRL + REG_CLR);
	readl(lcdif->base + LCDC_V8_CTRL);
}

static void lcdif_crtc_mode_set_nofb(struct lcdif_drm_private *lcdif,
				     struct drm_display_mode *m,
				     struct lcdif_crtc_state *lcdif_crtc_state)
{
	dev_dbg(lcdif->dev, "Pixel clock: %dkHz (actual: %dkHz)\n",
			     m->clock, (int)(clk_get_rate(lcdif->clk) / 1000));
	dev_dbg(lcdif->dev, "Bridge bus_flags: 0x%08X\n",
			     lcdif_crtc_state->bus_flags);
	dev_dbg(lcdif->dev, "Mode flags: 0x%08X\n", m->flags);

	/* Mandatory eLCDIF reset as per the Reference Manual */
	lcdif_reset_block(lcdif);

	/* NOTE: This driver is currently fixed to DRM_FORMAT_XRGB8888 */
	lcdif_set_formats(lcdif, DRM_FORMAT_XRGB8888, lcdif_crtc_state->bus_format);

	lcdif_set_mode(lcdif, m, lcdif_crtc_state->bus_flags);
}

static void lcdif_crtc_atomic_flush(struct fb_info *info)
{
	struct lcdif_drm_private *lcdif = container_of(info, struct lcdif_drm_private, info);
	u32 reg;

	reg = readl(lcdif->base + LCDC_V8_CTRLDESCL0_5);
	reg |= CTRLDESCL0_5_SHADOW_LOAD_EN;
	writel(reg, lcdif->base + LCDC_V8_CTRLDESCL0_5);
}

static void lcdif_crtc_atomic_enable(struct lcdif_drm_private *lcdif,
				     struct drm_display_mode *mode,
				     struct lcdif_crtc_state *vcstate)
{
	dma_addr_t paddr;
	u32 reg;

	clk_set_rate(lcdif->clk, mode->clock * 1000);

	lcdif_crtc_mode_set_nofb(lcdif, mode, vcstate);

	/* Write cur_buf as well to avoid an initial corrupt frame */
	paddr = lcdif->paddr;
	if (paddr) {
		writel(lower_32_bits(paddr),
		       lcdif->base + LCDC_V8_CTRLDESCL_LOW0_4);
		writel(CTRLDESCL_HIGH0_4_ADDR_HIGH(upper_32_bits(paddr)),
		       lcdif->base + LCDC_V8_CTRLDESCL_HIGH0_4);
		/* initial flush of the current data */
		reg = readl(lcdif->base + LCDC_V8_CTRLDESCL0_5);
		reg |= CTRLDESCL0_5_SHADOW_LOAD_EN;
		writel(reg, lcdif->base + LCDC_V8_CTRLDESCL0_5);
	}
	lcdif_enable_controller(lcdif);
}

static void lcdif_enable_fb_controller(struct fb_info *info)
{
	struct lcdif_drm_private *lcdif = container_of(info, struct lcdif_drm_private, info);
	struct drm_display_mode mode = {};
	struct lcdif_crtc_state vcstate = {
		.bus_format = 0,
		.bus_flags = 0,
	};
	struct drm_display_info display_info = {};
	int ret;

	if (!info->mode) {
		dev_err(lcdif->dev, "no modes, cannot enable\n");
		return;
	}

	fb_videomode_to_drm_display_mode(info->mode, &mode);

	ret = vpl_ioctl(&lcdif->vpl, lcdif->id, VPL_GET_BUS_FORMAT, &vcstate.bus_format);
	if (ret < 0) {
		dev_err(lcdif->dev, "Cannot determine bus format\n");
		return;
	}

	ret = vpl_ioctl(&lcdif->vpl, lcdif->id, VPL_GET_DISPLAY_INFO, &display_info);
	if (ret < 0) {
		dev_err(lcdif->dev, "Cannot get display info\n");
		return;
	}

	vcstate.bus_flags = display_info.bus_flags;

	dev_info(lcdif->dev, "vp%d: bus_format: 0x%08x bus_flags: 0x%08x\n",
		 lcdif->id, vcstate.bus_format, display_info.bus_flags);

	vpl_ioctl_prepare(&lcdif->vpl, lcdif->id, info->mode);

	lcdif_crtc_atomic_enable(lcdif, &mode, &vcstate);

	vpl_ioctl_enable(&lcdif->vpl, lcdif->id);
}

static void lcdif_disable_fb_controller(struct fb_info *info)
{
	struct lcdif_drm_private *lcdif = container_of(info, struct lcdif_drm_private, info);

	lcdif_disable_controller(lcdif);
}

static struct fb_ops lcdif_fb_ops = {
	.fb_enable = lcdif_enable_fb_controller,
	.fb_disable = lcdif_disable_fb_controller,
	.fb_flush = lcdif_crtc_atomic_flush,
};

/* -----------------------------------------------------------------------------
 * Initialization
 */

static struct fb_bitfield red    = { .offset = 16, .length = 8, };
static struct fb_bitfield green  = { .offset =  8, .length = 8, };
static struct fb_bitfield blue   = { .offset =  0, .length = 8, };
static struct fb_bitfield transp = { .offset = 24, .length = 8, };

static int lcdif_register_fb(struct lcdif_drm_private *lcdif)
{
	struct fb_info *info = &lcdif->info;
	u32 xmax = 0, ymax = 0;
	int i, ret;

	info->fbops = &lcdif_fb_ops;
	info->bits_per_pixel = 32;
	info->red = red;
	info->green = green;
	info->blue = blue;
	info->transp = transp;
	info->dev.parent = lcdif->dev;

	ret = vpl_ioctl(&lcdif->vpl, 0, VPL_GET_VIDEOMODES, &info->modes);
	if (ret) {
		dev_err(lcdif->dev, "failed to get modes: %s\n", strerror(-ret));
		return ret;
	}

	if (info->modes.num_modes) {
		for (i = 0; i < info->modes.num_modes; i++) {
			xmax = max(xmax, info->modes.modes[i].xres);
			ymax = max(ymax, info->modes.modes[i].yres);
		}
		info->xres = info->modes.modes[info->modes.native_mode].xres;
		info->yres = info->modes.modes[info->modes.native_mode].yres;
	} else {
		dev_notice(lcdif->dev, "no modes found on lcdif%d\n", lcdif->id);
		xmax = info->xres = 640;
		ymax = info->yres = 480;
	}

	lcdif->line_length = xmax * (info->bits_per_pixel >> 3);
	lcdif->max_yres = ymax;

	info->line_length = lcdif->line_length;
	info->screen_base = dma_alloc_writecombine(DMA_DEVICE_BROKEN,
				info->line_length * lcdif->max_yres,
				&lcdif->paddr);

	if (!info->screen_base)
		return -ENOMEM;

	ret = register_framebuffer(info);
	if (ret)
		return ret;

	dev_info(lcdif->dev, "Registered %s on LCDIF%d, type primary\n",
		 info->cdev.name, lcdif->id);

	return 0;
}

int lcdif_kms_init(struct lcdif_drm_private *lcdif)
{
	struct device *dev;
	struct device_node *port;
	struct device_node *ep;
	struct of_endpoint endpoint;
	int ret;

	dev = lcdif->dev;

	port = of_graph_get_port_by_id(dev->of_node, 0);
	if (!port) {
		dev_err(lcdif->dev, "no port node found for video_port0\n");
		return -ENOENT;
	}

	for_each_child_of_node(port, ep) {
		of_graph_parse_endpoint(ep, &endpoint);
		lcdif->crtc_endpoint_id = endpoint.id;
	}

	lcdif->port = port;
	lcdif->vpl.node = dev->of_node;

	ret = vpl_register(&lcdif->vpl);
	if (ret)
		return ret;

	lcdif_register_fb(lcdif);

	return 0;
}
