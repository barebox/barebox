// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * TI Omap Frame Buffer device driver
 *
 * Copyright (C) 2013 Christoph Fritz <chf.fritz@googlemail.com>
 *   Based on work by Enrico Scholz, sponsored by Phytec
 */

#include <driver.h>
#include <dma.h>
#include <fb.h>
#include <errno.h>
#include <xfuncs.h>
#include <init.h>
#include <stdio.h>
#include <io.h>
#include <common.h>
#include <malloc.h>
#include <clock.h>
#include <linux/err.h>

#include <video/omap-fb.h>

#include <mmu.h>

#include "omap.h"

struct omapfb_device {
	struct fb_info info;
	struct device *dev;

	struct omapfb_display const *cur_display;

	struct omapfb_display const *displays;
	size_t num_displays;

	void __iomem *dss;
	void __iomem *dispc;

	struct {
		void __iomem *addr;
		size_t size;
	} prealloc_screen;

	struct {
		uint32_t dispc_control;
		uint32_t dispc_pol_freq;
	} shadow;

	struct {
		unsigned int dss_clk_hz;
		unsigned int lckd;
		unsigned int pckd;
	} divisor;
	size_t dma_size;
	void (*enable_fn)(int);

	struct fb_videomode	video_modes[];
};

static inline struct omapfb_device *to_omapfb(const struct fb_info *info)
{
	return container_of(info, struct omapfb_device, info);
}

static void omapfb_enable(struct fb_info *info)
{
	struct omapfb_device *fbi = to_omapfb(info);

	dev_dbg(fbi->dev, "%s\n", __func__);

	if (!fbi->cur_display) {
		dev_err(fbi->dev, "no valid mode set\n");
		return;
	}

	if (fbi->enable_fn)
		fbi->enable_fn(1);

	udelay(fbi->cur_display->power_on_delay * 1000u);

	o4_dispc_write(o4_dispc_read(O4_DISPC_CONTROL2) |
		DSS_DISPC_CONTROL_LCDENABLE |
		DSS_DISPC_CONTROL_LCDENABLESIGNAL, O4_DISPC_CONTROL2);

	o4_dispc_write(o4_dispc_read(O4_DISPC_VID1_ATTRIBUTES) |
		DSS_DISPC_VIDn_ATTRIBUTES_VIDENABLE, O4_DISPC_VID1_ATTRIBUTES);

	o4_dispc_write(o4_dispc_read(O4_DISPC_CONTROL2) |
		DSS_DISPC_CONTROL_GOLCD, O4_DISPC_CONTROL2);
}

static void omapfb_disable(struct fb_info *info)
{
	struct omapfb_device *fbi = to_omapfb(info);

	dev_dbg(fbi->dev, "%s\n", __func__);

	if (!fbi->cur_display) {
		dev_err(fbi->dev, "no valid mode set\n");
		return;
	}

	o4_dispc_write(o4_dispc_read(O4_DISPC_CONTROL2) &
		~(DSS_DISPC_CONTROL_LCDENABLE |
		DSS_DISPC_CONTROL_LCDENABLESIGNAL), O4_DISPC_CONTROL2);

	o4_dispc_write(o4_dispc_read(O4_DISPC_VID1_ATTRIBUTES) &
		~(DSS_DISPC_VIDn_ATTRIBUTES_VIDENABLE),
		O4_DISPC_VID1_ATTRIBUTES);

	if (fbi->prealloc_screen.addr == NULL) {
		/* free frame buffer; but only when screen is not
		* preallocated */
		if (info->screen_base)
			dma_free_coherent(DMA_DEVICE_BROKEN,
					  info->screen_base, 0, fbi->dma_size);
	}

	info->screen_base = NULL;

	udelay(fbi->cur_display->power_off_delay * 1000u);

	if (fbi->enable_fn)
		fbi->enable_fn(0);
}

static void omapfb_calc_divisor(struct omapfb_device *fbi,
			struct fb_videomode const *mode)
{
	unsigned int l, k, t, b;

	b = UINT_MAX;
	for (l = 1; l < 256; l++) {
		for (k = 1; k < 256; k++) {
			t = abs(mode->pixclock * 100 -
				(fbi->divisor.dss_clk_hz / l / k));
			if (t <= b) {
				b = t;
				fbi->divisor.lckd = l;
				fbi->divisor.pckd = k;
			}
		}
	}
}

static unsigned int omapfb_calc_format(struct fb_info const *info)
{
	struct omapfb_device *fbi = to_omapfb(info);

	switch (info->bits_per_pixel) {
	case 24:
		return 9;
	case 32:
		return 0x8; /* xRGB24-8888 (32-bit container) */
	default:
		dev_err(fbi->dev, "%s: unsupported bpp %d\n", __func__,
			info->bits_per_pixel);
		return 0;
	}
}

struct omapfb_colors {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

static struct omapfb_colors const omapfb_col[] = {
	[0] = {
		.red	= { .length = 0, .offset = 0 },
	},
	[1] = {
		.blue	= { .length = 8, .offset = 0 },
		.green	= { .length = 8, .offset = 8 },
		.red	= { .length = 8, .offset = 16 },
	},
	[2] = {
		.blue	= { .length = 8, .offset = 0 },
		.green	= { .length = 8, .offset = 8 },
		.red	= { .length = 8, .offset = 16 },
		.transp	= { .length = 8, .offset = 24 },
	},
};

static void omapfb_fill_shadow(struct omapfb_device *fbi,
				struct omapfb_display const *display)
{
	fbi->shadow.dispc_control = 0;
	fbi->shadow.dispc_pol_freq = 0;

	fbi->shadow.dispc_control |= DSS_DISPC_CONTROL_STNTFT;

	switch (display->config & OMAP_DSS_LCD_DATALINES_msk) {
	case OMAP_DSS_LCD_DATALINES_12:
		fbi->shadow.dispc_control |= DSS_DISPC_CONTROL_TFTDATALINES_12;
		break;
	case OMAP_DSS_LCD_DATALINES_16:
		fbi->shadow.dispc_control |= DSS_DISPC_CONTROL_TFTDATALINES_16;
		break;
	case OMAP_DSS_LCD_DATALINES_18:
		fbi->shadow.dispc_control |= DSS_DISPC_CONTROL_TFTDATALINES_18;
		break;
	case OMAP_DSS_LCD_DATALINES_24:
		fbi->shadow.dispc_control |= DSS_DISPC_CONTROL_TFTDATALINES_24;
		break;
	}

	if (display->config & OMAP_DSS_LCD_IPC)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_IPC;

	if (display->config & OMAP_DSS_LCD_IVS)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_IVS;

	if (display->config & OMAP_DSS_LCD_IHS)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_IHS;

	if (display->config & OMAP_DSS_LCD_IEO)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_IEO;

	if (display->config & OMAP_DSS_LCD_RF)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_RF;

	if (display->config & OMAP_DSS_LCD_ONOFF)
		fbi->shadow.dispc_pol_freq |= DSS_DISPC_POL_FREQ_ONOFF;
}

static int omapfb_find_display_by_name(struct omapfb_device *fbi,
		const char *name)
{
	int i;

	for (i = 0; i < fbi->num_displays; ++i) {
		if (strcmp(name, fbi->displays[i].mode.name) == 0)
			return i;
	}
	return -ENXIO;
}

static int omapfb_activate_var(struct fb_info *info)
{
	struct omapfb_device *fbi = to_omapfb(info);
	struct fb_videomode const *mode = info->mode;
	size_t size = mode->xres * mode->yres * (info->bits_per_pixel / 8);
	int rc;
	unsigned int fmt = omapfb_calc_format(info);
	struct omapfb_colors const *cols;
	struct omapfb_display const *new_display = NULL;

	rc = omapfb_find_display_by_name(fbi, mode->name);
	if (rc < 0) {
		dev_err(fbi->dev, "no display found for this mode '%s'\n",
			mode->name);
		goto out;
	}
	new_display = &fbi->displays[rc];

	/*Free old screen buf*/
	if (!fbi->prealloc_screen.addr && info->screen_base)
		dma_free_coherent(DMA_DEVICE_BROKEN,
				  info->screen_base, 0, fbi->dma_size);

	fbi->dma_size = PAGE_ALIGN(size);

	if (!fbi->prealloc_screen.addr) {
		/* case 1: no preallocated screen */
		info->screen_base = dma_alloc_coherent(DMA_DEVICE_BROKEN,
						       size, DMA_ADDRESS_BROKEN);
	} else if (fbi->prealloc_screen.size < fbi->dma_size) {
		/* case 2: preallocated screen, but too small */
		dev_err(fbi->dev,
			"allocated framebuffer too small (%zu < %zu)\n",
			fbi->prealloc_screen.size, fbi->dma_size);
		rc = -ENOMEM;
		goto out;
	} else {
		/* case 3: preallocated screen */
		info->screen_base = fbi->prealloc_screen.addr;
	}

	omapfb_fill_shadow(fbi, new_display);

	omapfb_calc_divisor(fbi, mode);

	switch (info->bits_per_pixel) {
	case 24:
		cols = &omapfb_col[1];
		break;
	case 32:
		cols = &omapfb_col[2];
		break;
	default:
		cols = &omapfb_col[0];
	}

	info->red = cols->red;
	info->green = cols->green;
	info->blue = cols->blue;
	info->transp = cols->transp;

	o4_dispc_write(fbi->shadow.dispc_control, O4_DISPC_CONTROL2);

	o4_dispc_write(fbi->shadow.dispc_pol_freq, O4_DISPC_POL_FREQ2);

	o4_dispc_write(DSS_DISPC_TIMING_H_HSW(mode->hsync_len - 1) |
		DSS_DISPC_TIMING_H_HFP(mode->right_margin - 1) |
		DSS_DISPC_TIMING_H_HBP(mode->left_margin - 1),
		O4_DISPC_TIMING_H2);

	o4_dispc_write(DSS_DISPC_TIMING_V_VSW(mode->vsync_len - 1) |
		DSS_DISPC_TIMING_V_VFP(mode->lower_margin) |
		DSS_DISPC_TIMING_V_VBP(mode->upper_margin), O4_DISPC_TIMING_V2);

	o4_dispc_write(DSS_DISPC_DIVISOR_ENABLE | DSS_DISPC_DIVISOR_LCD(1),
		O4_DISPC_DIVISOR);

	o4_dispc_write(DSS_DISPC_DIVISOR2_LCD(fbi->divisor.lckd) |
		DSS_DISPC_DIVISOR2_PCD(fbi->divisor.pckd), O4_DISPC_DIVISOR2);

	o4_dispc_write(DSS_DISPC_SIZE_LCD_PPL(mode->xres - 1) |
		DSS_DISPC_SIZE_LCD_LPP(mode->yres - 1), O4_DISPC_SIZE_LCD2);

	o4_dispc_write(0x0000ff00, O4_DISPC_DEFAULT_COLOR2);

	/* we use VID1 */
	o4_dispc_write((uintptr_t)info->screen_base, O4_DISPC_VID1_BA0);
	o4_dispc_write((uintptr_t)info->screen_base, O4_DISPC_VID1_BA1);

	o4_dispc_write(DSS_DISPC_VIDn_POSITION_VIDPOSX(0) |
		DSS_DISPC_VIDn_POSITION_VIDPOSY(0), O4_DISPC_VID1_POSITION);
	o4_dispc_write(DSS_DISPC_VIDn_SIZE_VIDSIZEX(mode->xres - 1) |
		DSS_DISPC_VIDn_SIZE_VIDSIZEY(mode->yres - 1),
		O4_DISPC_VID1_SIZE);
	o4_dispc_write(DSS_DISPC_VIDn_PICTURE_SIZE_VIDORGSIZEX(mode->xres - 1) |
		DSS_DISPC_VIDn_PICTURE_SIZE_VIDORGSIZEY(mode->yres - 1),
		O4_DISPC_VID1_PICTURE_SIZE);
	o4_dispc_write(1, O4_DISPC_VID1_ROW_INC);
	o4_dispc_write(1, O4_DISPC_VID1_PIXEL_INC);

	o4_dispc_write(0xfff, O4_DISPC_VID1_PRELOAD);

	o4_dispc_write(DSS_DISPC_VIDn_ATTRIBUTES_VIDFORMAT(fmt) |
		DSS_DISPC_VIDn_ATTRIBUTES_VIDBURSTSIZE_8x128 |
		DSS_DISPC_VIDn_ATTRIBUTES_ZORDERENABLE |
		DSS_DISPC_VIDn_ATTRIBUTES_CHANNELOUT2_SECONDARY_LCD,
		O4_DISPC_VID1_ATTRIBUTES);

	rc = wait_on_timeout(OFB_TIMEOUT,
		!(o4_dispc_read(O4_DISPC_CONTROL2) &
		DSS_DISPC_CONTROL_GOLCD));

	if (rc) {
		dev_err(fbi->dev, "timeout: dispc golcd\n");
		goto out;
	}

	o4_dispc_write(o4_dispc_read(O4_DISPC_CONTROL2) |
		DSS_DISPC_CONTROL_GOLCD, O4_DISPC_CONTROL2);

	fbi->cur_display = new_display;
	info->xres = mode->xres;
	info->yres = mode->yres;

	rc = 0;

out:
	return rc;
}

static int omapfb_reset(struct omapfb_device const *fbi)
{
	uint32_t v = o4_dispc_read(O4_DISPC_CONTROL2);
	int rc;

	/* step 1: stop the LCD controller */
	if (v & DSS_DISPC_CONTROL_LCDENABLE) {
		o4_dispc_write(v & ~DSS_DISPC_CONTROL_LCDENABLE,
			O4_DISPC_CONTROL2);

		o4_dispc_write(DSS_DISPC_IRQSTATUS_FRAMEDONE2,
			O4_DISPC_IRQSTATUS);

		rc = wait_on_timeout(OFB_TIMEOUT,
			((o4_dispc_read(O4_DISPC_IRQSTATUS) &
			DSS_DISPC_IRQSTATUS_FRAMEDONE) != 0));

		if (rc) {
			dev_err(fbi->dev, "timeout: irqstatus framedone\n");
			return -ETIMEDOUT;
		}
	}

	/* step 2: wait for reset done status */
	rc = wait_on_timeout(OFB_TIMEOUT,
		(o4_dss_read(O4_DSS_SYSSTATUS) &
		DSS_DSS_SYSSTATUS_RESETDONE));

	if (rc) {
		dev_err(fbi->dev, "timeout: sysstatus resetdone\n");
		return -ETIMEDOUT;
	}

	/* DSS_CTL: set to reset value */
	o4_dss_write(0, O4_DSS_CTRL);

	return 0;
}

static struct fb_ops omapfb_ops = {
	.fb_enable		= omapfb_enable,
	.fb_disable		= omapfb_disable,
	.fb_activate_var	= omapfb_activate_var,
};

static int omapfb_probe(struct device *dev)
{
	struct omapfb_platform_data const *pdata = dev->platform_data;
	struct omapfb_device *fbi;
	struct fb_info *info;
	int rc;
	size_t i;

	fbi = xzalloc(sizeof *fbi +
		pdata->num_displays * sizeof fbi->video_modes[0]);
	info = &fbi->info;

	fbi->dev = dev;

	/* CM_DSS_CLKSTCTRL (TRM: 935) trigger SW_WKUP */
	__raw_writel(0x2, 0x4a009100); /* TODO: move this to clockmanagement */

	fbi->dss   = dev_request_mem_region_by_name(dev, "omap4_dss");
	fbi->dispc = dev_request_mem_region_by_name(dev, "omap4_dispc");

	if (IS_ERR(fbi->dss) || IS_ERR(fbi->dispc)) {
		dev_err(dev, "Insufficient register description\n");
		rc = -EINVAL;
		goto out;
	}

	dev_info(dev, "HW-Revision 0x%04x 0x%04x\n",
		o4_dss_read(O4_DISPC_REVISION),
		o4_dss_read(O4_DSS_REVISION));

	if (!pdata->dss_clk_hz | !pdata->displays | !pdata->num_displays |
		!pdata->bpp) {
		dev_err(dev, "Insufficient omapfb_platform_data\n");
		rc = -EINVAL;
		goto out;
	}

	fbi->enable_fn    = pdata->enable;
	fbi->displays     = pdata->displays;
	fbi->num_displays = pdata->num_displays;
	fbi->divisor.dss_clk_hz = pdata->dss_clk_hz;

	for (i = 0; i < pdata->num_displays; ++i)
		fbi->video_modes[i] = pdata->displays[i].mode;

	info->modes.modes = fbi->video_modes;
	info->modes.num_modes = pdata->num_displays;

	info->priv = fbi;
	info->fbops = &omapfb_ops;
	info->bits_per_pixel = pdata->bpp;

	if (pdata->screen) {
		if (!IS_ALIGNED(pdata->screen->start, PAGE_SIZE) ||
			!IS_ALIGNED(resource_size(pdata->screen), PAGE_SIZE)) {
			dev_err(dev, "screen resource not aligned\n");
			rc = -EINVAL;
			goto out;
		}
		fbi->prealloc_screen.addr =
				(void __iomem *)pdata->screen->start;
		fbi->prealloc_screen.size = resource_size(pdata->screen);
		remap_range(fbi->prealloc_screen.addr,
			fbi->prealloc_screen.size, MAP_UNCACHED);
	}

	rc = omapfb_reset(fbi);
	if (rc < 0) {
		dev_err(dev, "failed to reset: %d\n", rc);
		goto out;
	}

	info->dev.parent = dev;
	rc = register_framebuffer(info);
	if (rc < 0) {
		dev_err(dev, "failed to register framebuffer: %d\n", rc);
		goto out;
	}

	rc = 0;
	dev_info(dev, "registered\n");

out:
	if (rc < 0)
		free(fbi);

	return rc;
}

static struct driver omapfb_driver = {
	.name	= "omap_fb",
	.probe	= omapfb_probe,
};

device_platform_driver(omapfb_driver);
