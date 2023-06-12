// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017-2018 STMicroelectronics - All Rights Reserved
 * Author(s): Philippe Cornu <philippe.cornu@st.com> for STMicroelectronics.
 *	      Yannick Fertre <yannick.fertre@st.com> for STMicroelectronics.
 *	      Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <io.h>
#include <fb.h>
#include <dma.h>
#include <video/media-bus-format.h>
#include <video/vpl.h>
#include <of_graph.h>

#include "stm32_ltdc.h"

struct ltdc_hw {
	void __iomem *regs;
	struct device *dev;
	struct clk *pclk;
	bool claimed;
};

struct ltdc_fb {
	int id;
	struct fb_info info;
	u32 bg_col_argb;
	u32 alpha;
	u32 bus_format;
	enum stm32_ltdc_pixfmt pixfmt;
	struct vpl vpl;
	struct ltdc_hw *hw;
};

static bool has_alpha(enum stm32_ltdc_pixfmt pixfmt)
{
	switch (pixfmt) {
	case PF_ARGB8888:
	case PF_ARGB1555:
	case PF_ARGB4444:
	case PF_AL44:
	case PF_AL88:
		return true;
	case PF_RGB888:
	case PF_RGB565:
	case PF_L8:
	default:
		return false;
	}
}

static void ltdc_set_mode(struct ltdc_fb *priv,
			  struct fb_videomode *mode)
{
	void __iomem *regs = priv->hw->regs;
	u32 hsync, vsync, acc_hbp, acc_vbp, acc_act_w, acc_act_h;
	u32 total_w, total_h;
	u32 val;

	/* Convert video timings to ltdc timings */
	hsync = mode->hsync_len - 1;
	vsync = mode->vsync_len - 1;
	acc_hbp = hsync + mode->left_margin;
	acc_vbp = vsync + mode->upper_margin;
	acc_act_w = acc_hbp + mode->xres;
	acc_act_h = acc_vbp + mode->yres;
	total_w = acc_act_w + mode->right_margin;
	total_h = acc_act_h + mode->lower_margin;

	/* Synchronization sizes */
	val = (hsync << 16) | vsync;
	clrsetbits_le32(regs + LTDC_SSCR, SSCR_VSH | SSCR_HSW, val);

	/* Accumulated back porch */
	val = (acc_hbp << 16) | acc_vbp;
	clrsetbits_le32(regs + LTDC_BPCR, BPCR_AVBP | BPCR_AHBP, val);

	/* Accumulated active width */
	val = (acc_act_w << 16) | acc_act_h;
	clrsetbits_le32(regs + LTDC_AWCR, AWCR_AAW | AWCR_AAH, val);

	/* Total width & height */
	val = (total_w << 16) | total_h;
	clrsetbits_le32(regs + LTDC_TWCR, TWCR_TOTALH | TWCR_TOTALW, val);

	setbits_le32(regs + LTDC_LIPCR, acc_act_h + 1);

	/* Signal polarities */
	val = 0;
	dev_dbg(priv->hw->dev, "mode->display_flags 0x%x mode->sync 0x%x\n",
		mode->display_flags, mode->sync);
	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		val |= GCR_HSPOL;
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		val |= GCR_VSPOL;
	if (mode->display_flags & DISPLAY_FLAGS_DE_LOW)
		val |= GCR_DEPOL;
	if (mode->display_flags & DISPLAY_FLAGS_PIXDATA_NEGEDGE)
		val |= GCR_PCPOL;

	clrsetbits_le32(regs + LTDC_GCR,
			GCR_HSPOL | GCR_VSPOL | GCR_DEPOL | GCR_PCPOL, val);

	/* Overall background color */
	writel(priv->bg_col_argb, regs + LTDC_BCCR);
}

static void ltdc_set_layer1(struct ltdc_fb *priv)
{
	void __iomem *regs = priv->hw->regs;
	u32 x0, x1, y0, y1;
	u32 pitch_in_bytes;
	u32 line_length;
	u32 bus_width;
	u32 val, tmp, bpp;
	struct fb_videomode *mode = priv->info.mode;

	x0 = y0 = 0;
	x1 = mode->xres - 1;
	y1 = mode->yres - 1;

	/* Horizontal start and stop position */
	tmp = (readl(regs + LTDC_BPCR) & BPCR_AHBP) >> 16;
	val = ((x1 + 1 + tmp) << 16) + (x0 + 1 + tmp);
	clrsetbits_le32(regs + LTDC_L1WHPCR, LXWHPCR_WHSTPOS | LXWHPCR_WHSPPOS,
			val);

	/* Vertical start & stop position */
	tmp = readl(regs + LTDC_BPCR) & BPCR_AVBP;
	val = ((y1 + 1 + tmp) << 16) + (y0 + 1 + tmp);
	clrsetbits_le32(regs + LTDC_L1WVPCR, LXWVPCR_WVSTPOS | LXWVPCR_WVSPPOS,
			val);

	/* Layer background color */
	writel(priv->bg_col_argb, regs + LTDC_L1DCCR);

	/* Color frame buffer pitch in bytes & line length */
	bpp = priv->info.bits_per_pixel;
	pitch_in_bytes = mode->xres * (bpp >> 3);
	bus_width = 8 << ((readl(regs + LTDC_GC2R) & GC2R_BW) >> 4);
	line_length = ((bpp >> 3) * mode->xres) + (bus_width >> 3) - 1;
	val = (pitch_in_bytes << 16) | line_length;
	clrsetbits_le32(regs + LTDC_L1CFBLR, LXCFBLR_CFBLL | LXCFBLR_CFBP, val);

	/* Pixel format */
	clrsetbits_le32(regs + LTDC_L1PFCR, LXPFCR_PF, priv->pixfmt);

	/* Constant alpha value */
	clrsetbits_le32(regs + LTDC_L1CACR, LXCACR_CONSTA, priv->alpha);

	/* Specifies the blending factors : with or without pixel alpha */
	/* Manage hw-specific capabilities */
	val = has_alpha(priv->pixfmt) ? BF1_PAXCA | BF2_1PAXCA : BF1_CA | BF2_1CA;

	/* Blending factors */
	clrsetbits_le32(regs + LTDC_L1BFCR, LXBFCR_BF2 | LXBFCR_BF1, val);

	/* Frame buffer line number */
	clrsetbits_le32(regs + LTDC_L1CFBLNR, LXCFBLNR_CFBLN, mode->yres);

	/* Frame buffer address */
	writel((unsigned long)priv->info.screen_base, regs + LTDC_L1CFBAR);

	/* Enable layer 1 */
	setbits_le32(regs + LTDC_L1CR, LXCR_LEN);
}

static int ltdc_activate_var(struct fb_info *info)
{
	info->line_length = info->xres * (info->bits_per_pixel >> 3);

	info->screen_base = dma_alloc_writecombine(info->line_length * info->yres,
						   DMA_ADDRESS_BROKEN);
	if (!info->screen_base)
		return -ENOMEM;

	return 0;
}

static void ltdc_enable(struct fb_info *info)
{
	struct fb_videomode *mode = info->mode;
	struct ltdc_fb *priv = info->priv;
	struct ltdc_hw *hw = priv->hw;
	u32 pixclock;
	int ret;

	if (hw->claimed) {
		dev_warn(hw->dev, "CRTC currently claimed by other frame buffer!\n");
		return;
	}

	vpl_ioctl_prepare(&priv->vpl, priv->id, mode);

	pixclock = PICOS2KHZ(mode->pixclock) * 1000;

	ret = clk_enable(hw->pclk);
	if (ret) {
		dev_err(hw->dev, "peripheral clock enable error %d\n", ret);
		return;
	}

	clk_set_rate(clk_get_parent(hw->pclk), pixclock);
	if (!ret)
		ret = clk_set_rate(hw->pclk, pixclock);
	if (ret < 0) {
		dev_err(hw->dev, "fail to set pixel clock %d hz: %d\n",
			pixclock, ret);
		return;
	}

	ret = device_reset_us(hw->dev, 100000);
	if (ret) {
		dev_err(hw->dev, "error resetting controller %d\n", ret);
		return;
	}

	/* Configure & start LTDC */
	ltdc_set_mode(priv, mode);
	ltdc_set_layer1(priv);

	/* Reload configuration immediately & enable LTDC */
	setbits_le32(hw->regs + LTDC_SRCR, SRCR_IMR);
	setbits_le32(hw->regs + LTDC_GCR, GCR_LTDCEN);

	vpl_ioctl_enable(&priv->vpl, priv->id);

	hw->claimed = true;
}

static void ltdc_disable(struct fb_info *info)
{
	struct ltdc_fb *priv = info->priv;

	vpl_ioctl_disable(&priv->vpl, priv->id);

	clrbits_le32(priv->hw->regs + LTDC_GCR, GCR_LTDCEN);
	clk_disable(priv->hw->pclk);
	priv->hw->claimed = false;

	vpl_ioctl_unprepare(&priv->vpl, priv->id);
}

static struct fb_ops ltdc_ops = {
	.fb_activate_var	= ltdc_activate_var,
	.fb_enable		= ltdc_enable,
	.fb_disable		= ltdc_disable,
};

static int ltdc_probe(struct device *dev)
{
	struct device_node *np;
	struct resource *iores;
	struct ltdc_hw *hw;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	hw = xzalloc(sizeof *hw);
	hw->dev = dev;
	hw->regs = IOMEM(iores->start);

	hw->pclk = clk_get(dev, NULL);
	if (IS_ERR(hw->pclk)) {
		dev_err(dev, "peripheral clock get error %d\n", ret);
		return PTR_ERR(hw->pclk);
	}

	for_each_available_child_of_node(dev->of_node, np) {
		struct ltdc_fb *priv;
		struct of_endpoint ep;
		struct fb_info *info;

		if (!of_graph_port_is_available(np))
			continue;

		ret = of_graph_parse_endpoint(np, &ep);
		if (ret)
			return ret;

		dev_dbg(hw->dev, "register vpl for %s\n", np->full_name);

		priv = xzalloc(sizeof(*priv));
		priv->hw = hw;
		priv->id = ep.id;
		priv->vpl.node = dev->of_node;

		ret = vpl_register(&priv->vpl);
		if (ret)
			return ret;

		info = &priv->info;
		info->priv = priv;
		info->fbops = &ltdc_ops;

		info->red	= (struct fb_bitfield){ .offset = 11, .length = 5, };
		info->green	= (struct fb_bitfield){ .offset =  5, .length = 6, };
		info->blue	= (struct fb_bitfield){ .offset =  0, .length = 5, };
		info->bits_per_pixel = 16,
		priv->pixfmt = PF_RGB565;
		/* TODO Below parameters are hard-coded for the moment... */
		priv->bg_col_argb = 0xFFFFFFFF; /* white no transparency */
		priv->alpha = 0xFF;

		ret = vpl_ioctl(&priv->vpl, priv->id, VPL_GET_VIDEOMODES, &info->modes);
		if (ret)
			dev_dbg(dev, "failed to get modes: %s\n", strerror(-ret));

		ret = register_framebuffer(info);
		if (ret < 0) {
			dev_err(dev, "failed to register framebuffer\n");
			return ret;
		}
	}

	return 0;
}

static __maybe_unused struct of_device_id ltdc_ids[] = {
	{ .compatible = "st,stm32-ltdc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ltdc_ids);

static struct driver ltdc_driver = {
	.name = "stm32-ltdc",
	.probe = ltdc_probe,
	.of_compatible = DRV_OF_COMPAT(ltdc_ids),
};
device_platform_driver(ltdc_driver);
