/*
 *  Freescale i.MX Frame Buffer device driver
 *
 *  Copyright (C) 2004 Sascha Hauer, Pengutronix
 *   Based on acornfb.c Copyright (C) Russell King.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */
#define pr_fmt(fmt) "IPU: " fmt

#include <common.h>
#include <dma.h>
#include <fb.h>
#include <io.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <of_graph.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>
#include <video/media-bus-format.h>

#include "imx-ipu-v3.h"
#include "ipuv3-plane.h"

/*
 * These are the bitfields for each
 * display depth that we support.
 */
struct ipufb_rgb {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

struct ipufb_info {
	void __iomem		*regs;

	struct clk		*ahb_clk;
	struct clk		*ipg_clk;
	struct clk		*per_clk;

	struct fb_videomode	*mode;

	struct fb_info		info;
	struct device_d		*dev;

	/* plane[0] is the full plane, plane[1] is the partial plane */
	struct ipu_plane	*plane[2];

	void			(*enable)(int enable);

	unsigned int		di_clkflags;
	u32			bus_format;
	struct ipu_dc		*dc;
	struct ipu_di		*di;

	struct list_head	list;
	char			*name;
	int			id;
	int			dino;

	struct vpl	vpl;
};

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int ipu_crtc_adjust_videomode(struct ipufb_info *fbi, struct fb_videomode *mode)
{
	u32 diff;

	if (mode->lower_margin >= 2)
		return 0;

	diff = 2 - mode->lower_margin;

	if (mode->upper_margin >= diff) {
		mode->upper_margin -= diff;
	} else if (mode->vsync_len > diff) {
		mode->vsync_len = mode->vsync_len - diff;
	} else {
		dev_warn(fbi->dev, "failed to adjust videomode\n");
		return -EINVAL;
	}

	mode->lower_margin = 2;

	dev_warn(fbi->dev, "videomode adapted for IPU restrictions\n");

	return 0;
}

int ipu_crtc_mode_set(struct ipufb_info *fbi,
			       struct fb_videomode *mode,
			       int x, int y)
{
	struct fb_info *info = &fbi->info;
	int ret;
	struct ipu_di_signal_cfg sig_cfg = {};
	struct ipu_di_mode di_mode = {};
	u32 bus_format = 0;

	dev_info(fbi->dev, "%s: mode->xres: %d\n", __func__,
			mode->xres);
	dev_info(fbi->dev, "%s: mode->yres: %d\n", __func__,
			mode->yres);

	vpl_ioctl(&fbi->vpl, 2 + fbi->dino, IMX_IPU_VPL_DI_MODE, &di_mode);
	vpl_ioctl(&fbi->vpl, 2 + fbi->dino, VPL_GET_BUS_FORMAT, &bus_format);
	if (bus_format)
		di_mode.di_clkflags = IPU_DI_CLKMODE_NON_FRACTIONAL;
	else
		bus_format = di_mode.bus_format ?: fbi->bus_format;

	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		sig_cfg.Hsync_pol = 1;
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		sig_cfg.Vsync_pol = 1;

	sig_cfg.enable_pol = !(mode->display_flags & DISPLAY_FLAGS_DE_LOW);
	/* Default to driving pixel data on negative clock edges */
	sig_cfg.clk_pol = !!(mode->display_flags & DISPLAY_FLAGS_PIXDATA_POSEDGE);
	sig_cfg.width = mode->xres;
	sig_cfg.height = mode->yres;
	sig_cfg.h_start_width = mode->left_margin;
	sig_cfg.h_sync_width = mode->hsync_len;
	sig_cfg.h_end_width = mode->right_margin;

	sig_cfg.v_start_width = mode->upper_margin;
	sig_cfg.v_sync_width = mode->vsync_len;

	ret = ipu_crtc_adjust_videomode(fbi, mode);
	if (ret)
		return ret;

	sig_cfg.v_end_width = mode->lower_margin;
	sig_cfg.pixelclock = PICOS2KHZ(mode->pixclock) * 1000UL;
	sig_cfg.clkflags = di_mode.di_clkflags;

	sig_cfg.v_to_h_sync = 0;

	sig_cfg.hsync_pin = 2;
	sig_cfg.vsync_pin = 3;

	ret = ipu_dc_init_sync(fbi->dc, fbi->di, sig_cfg.interlaced, bus_format,
			mode->xres);
	if (ret) {
		dev_err(fbi->dev,
				"initializing display controller failed with %d\n",
				ret);
		return ret;
	}

	ret = ipu_di_init_sync_panel(fbi->di, &sig_cfg);
	if (ret) {
		dev_err(fbi->dev,
				"initializing panel failed with %d\n", ret);
		return ret;
	}

	ret = ipu_plane_mode_set(fbi->plane[0], mode, info, DRM_FORMAT_XRGB8888,
				  0, 0, mode->xres, mode->yres,
				  x, y, mode->xres, mode->yres);
	if (ret) {
		dev_err(fbi->dev, "initialising plane failed with %s\n", strerror(-ret));
		return ret;
	}

	ret = ipu_di_enable(fbi->di);
	if (ret) {
		dev_err(fbi->dev, "enabling di failed with %s\n", strerror(-ret));
		return ret;
	}

	ipu_dc_enable_channel(fbi->dc);

	ipu_plane_enable(fbi->plane[0]);

	return 0;
}

static void ipufb_enable_controller(struct fb_info *info)
{
	struct ipufb_info *fbi = container_of(info, struct ipufb_info, info);

	if (!info->mode) {
		dev_err(fbi->dev, "No valid mode found\n");
		return;
	}

	vpl_ioctl_prepare(&fbi->vpl, 2 + fbi->dino, info->mode);

	ipu_crtc_mode_set(fbi, info->mode, 0, 0);

	vpl_ioctl_enable(&fbi->vpl, 2 + fbi->dino);
}

static void ipufb_disable_controller(struct fb_info *info)
{
	struct ipufb_info *fbi = container_of(info, struct ipufb_info, info);

	vpl_ioctl_disable(&fbi->vpl, 2 + fbi->dino);

	ipu_plane_disable(fbi->plane[0]);
	ipu_dc_disable_channel(fbi->dc);
	ipu_di_disable(fbi->di);

	vpl_ioctl_unprepare(&fbi->vpl, 2 + fbi->dino);
}

static int ipufb_activate_var(struct fb_info *info)
{
	struct ipufb_info *fbi = container_of(info, struct ipufb_info, info);

	info->line_length = info->xres * (info->bits_per_pixel >> 3);
	fbi->info.screen_base = dma_alloc_writecombine(info->line_length * info->yres,
						   DMA_ADDRESS_BROKEN);
	if (!fbi->info.screen_base)
		return -ENOMEM;

	memset(fbi->info.screen_base, 0x0, info->line_length * info->yres);

	return 0;
}

static struct fb_ops ipufb_ops = {
	.fb_enable	= ipufb_enable_controller,
	.fb_disable	= ipufb_disable_controller,
	.fb_activate_var = ipufb_activate_var,
};

static int ipu_get_resources(struct ipufb_info *fbi,
		struct ipu_client_platformdata *pdata)
{
	struct ipu_soc *ipu = fbi->dev->parent->priv;
	int ret;
	int dp = -EINVAL;

	fbi->dc = ipu_dc_get(ipu, pdata->dc);
	if (IS_ERR(fbi->dc)) {
		ret = PTR_ERR(fbi->dc);
		goto err_out;
	}

	fbi->di = ipu_di_get(ipu, pdata->di);
	if (IS_ERR(fbi->di)) {
		ret = PTR_ERR(fbi->di);
		goto err_out;
	}

	if (pdata->dp >= 0)
		dp = IPU_DP_FLOW_SYNC_BG;

	fbi->plane[0] = ipu_plane_init(ipu, pdata->dma[0], dp);
	ret = ipu_plane_get_resources(fbi->plane[0]);
	if (ret) {
		dev_err(fbi->dev, "getting plane 0 resources failed with %d.\n",
				ret);
		goto err_out;
	}

	return 0;
err_out:
	return ret;
}

static int ipufb_probe(struct device_d *dev)
{
	struct ipufb_info *fbi;
	struct fb_info *info;
	int ret, ipuid;
	struct ipu_client_platformdata *pdata = dev->platform_data;
	struct ipu_rgb *ipu_rgb;
	struct device_node *node;
	const char *fmt;

	fbi = xzalloc(sizeof(*fbi));
	info = &fbi->info;

	ipuid = of_alias_get_id(dev->parent->device_node, "ipu");
	fbi->name = basprintf("ipu%d-di%d", ipuid + 1, pdata->di);
	fbi->id = ipuid * 2 + pdata->di;
	fbi->dino = pdata->di;

	fbi->dev = dev;
	info->priv = fbi;
	info->fbops = &ipufb_ops;

	ipu_rgb = drm_fourcc_to_rgb(DRM_FORMAT_RGB888);

	info->bits_per_pixel = 32;
	info->red    = ipu_rgb->red;
	info->green  = ipu_rgb->green;
	info->blue   = ipu_rgb->blue;
	info->transp = ipu_rgb->transp;

	dev_dbg(dev, "i.MX Framebuffer driver\n");

	ret = ipu_get_resources(fbi, pdata);
	if (ret)
		return ret;

	node = of_graph_get_port_by_id(dev->parent->device_node, 2 + pdata->di);
	if (node && of_graph_port_is_available(node)) {
		dev_dbg(fbi->dev, "register vpl for %s\n", dev->parent->device_node->full_name);

		fbi->vpl.node = dev->parent->device_node;
		ret = vpl_register(&fbi->vpl);
		if (ret)
			return ret;

		ret = of_property_read_string(node, "interface-pix-fmt", &fmt);
		if (!ret) {
			if (!strcmp(fmt, "rgb24"))
				fbi->bus_format = MEDIA_BUS_FMT_RGB888_1X24;
			else if (!strcmp(fmt, "rgb565"))
				fbi->bus_format = MEDIA_BUS_FMT_RGB565_1X16;
			else if (!strcmp(fmt, "bgr666"))
				fbi->bus_format = MEDIA_BUS_FMT_RGB666_1X18;
			else if (!strcmp(fmt, "lvds666"))
				fbi->bus_format = MEDIA_BUS_FMT_RGB666_1X24_CPADHI;
		}

		ret = vpl_ioctl(&fbi->vpl, 2 + fbi->dino, VPL_GET_VIDEOMODES, &info->modes);
		if (ret) {
			dev_err(fbi->dev, "failed to get modes: %s\n", strerror(-ret));
			return ret;
		}

		ret = register_framebuffer(info);
		if (ret < 0) {
			dev_err(fbi->dev, "failed to register framebuffer\n");
			return ret;
		}
	}

	return ret;
}

static void ipufb_remove(struct device_d *dev)
{
}

static struct driver_d ipufb_driver = {
	.name		= "imx-ipuv3-crtc",
	.probe		= ipufb_probe,
	.remove		= ipufb_remove,
};

static int ipufb_register(void)
{
	return platform_driver_register(&ipufb_driver);
}
late_initcall(ipufb_register);
