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
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>

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
	u32			interface_pix_fmt;
	struct ipu_dc		*dc;
	struct ipu_di		*di;

	struct list_head	list;
	char			*name;
	int			id;

	struct ipu_output	*output;
};

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static LIST_HEAD(ipu_outputs);
static LIST_HEAD(ipu_fbs);

int ipu_register_output(struct ipu_output *ouput)
{
	list_add_tail(&ouput->list, &ipu_outputs);

	return 0;
}

static int ipu_register_fb(struct ipufb_info *ipufb)
{
	list_add_tail(&ipufb->list, &ipu_fbs);

	return 0;
}

int ipu_crtc_mode_set(struct ipufb_info *fbi,
			       struct fb_videomode *mode,
			       int x, int y)
{
	struct fb_info *info = &fbi->info;
	int ret;
	struct ipu_di_signal_cfg sig_cfg = {};
	u32 out_pixel_fmt;

	dev_info(fbi->dev, "%s: mode->xres: %d\n", __func__,
			mode->xres);
	dev_info(fbi->dev, "%s: mode->yres: %d\n", __func__,
			mode->yres);

	out_pixel_fmt = fbi->output->out_pixel_fmt;

	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		sig_cfg.Hsync_pol = 1;
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		sig_cfg.Vsync_pol = 1;

	sig_cfg.enable_pol = 1;
	sig_cfg.clk_pol = 1;
	sig_cfg.width = mode->xres;
	sig_cfg.height = mode->yres;
	sig_cfg.h_start_width = mode->left_margin;
	sig_cfg.h_sync_width = mode->hsync_len;
	sig_cfg.h_end_width = mode->right_margin;

	sig_cfg.v_start_width = mode->upper_margin;
	sig_cfg.v_sync_width = mode->vsync_len;
	sig_cfg.v_end_width = mode->lower_margin;
	sig_cfg.pixelclock = PICOS2KHZ(mode->pixclock) * 1000UL;
	sig_cfg.clkflags = fbi->output->di_clkflags;

	sig_cfg.v_to_h_sync = 0;

	sig_cfg.hsync_pin = 2;
	sig_cfg.vsync_pin = 3;

	ret = ipu_dc_init_sync(fbi->dc, fbi->di, sig_cfg.interlaced,
			out_pixel_fmt, mode->xres);
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
	struct ipu_output *output = fbi->output;

	if (output->ops->prepare)
		output->ops->prepare(output, info->mode, fbi->id);

	ipu_crtc_mode_set(fbi, info->mode, 0, 0);

	if (output->ops->enable)
		output->ops->enable(output, info->mode, fbi->id);
}

static void ipufb_disable_controller(struct fb_info *info)
{
	struct ipufb_info *fbi = container_of(info, struct ipufb_info, info);
	struct ipu_output *output = fbi->output;

	if (output->ops->disable)
		output->ops->disable(output);

	ipu_plane_disable(fbi->plane[0]);
	ipu_dc_disable_channel(fbi->dc);
	ipu_di_disable(fbi->di);

	if (output->ops->unprepare)
		output->ops->unprepare(output);
}

static int ipufb_activate_var(struct fb_info *info)
{
	struct ipufb_info *fbi = container_of(info, struct ipufb_info, info);

	info->line_length = info->xres * (info->bits_per_pixel >> 3);
	fbi->info.screen_base = dma_alloc_coherent(info->line_length * info->yres,
						   DMA_ADDRESS_BROKEN);
	if (!fbi->info.screen_base)
		return -ENOMEM;

	memset(fbi->info.screen_base, 0, info->line_length * info->yres);

	return 0;
}

static struct fb_ops ipufb_ops = {
	.fb_enable	= ipufb_enable_controller,
	.fb_disable	= ipufb_disable_controller,
	.fb_activate_var = ipufb_activate_var,
};

static struct ipufb_info *ipu_output_find_di(struct ipu_output *output)
{
	struct ipufb_info *ipufb;

	list_for_each_entry(ipufb, &ipu_fbs, list) {
		if (!(output->ipu_mask & (1 << ipufb->id)))
			continue;
		if (ipufb->output)
			continue;

		return ipufb;
	}

	return NULL;
}

static int ipu_init(void)
{
	struct ipu_output *output;
	struct ipufb_info *ipufb;
	int ret;

	list_for_each_entry(output, &ipu_outputs, list) {
		pr_info("found output: %s\n", output->name);
		ipufb = ipu_output_find_di(output);
		if (!ipufb) {
			pr_info("no di found for output %s\n", output->name);
			continue;
		}
		pr_info("using di %s for output %s\n", ipufb->name, output->name);

		ipufb->output = output;

		ipufb->info.edid_i2c_adapter = output->edid_i2c_adapter;
		if (output->modes) {
			ipufb->info.modes.modes = output->modes->modes;
			ipufb->info.modes.num_modes = output->modes->num_modes;
		}

		ret = register_framebuffer(&ipufb->info);
		if (ret < 0) {
			dev_err(ipufb->dev, "failed to register framebuffer\n");
			return ret;
		}
	}

	return 0;
}
late_initcall(ipu_init);

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

	fbi = xzalloc(sizeof(*fbi));
	info = &fbi->info;

	ipuid = of_alias_get_id(dev->parent->device_node, "ipu");
	fbi->name = asprintf("ipu%d-di%d", ipuid + 1, pdata->di);
	fbi->id = ipuid * 2 + pdata->di;

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

	ret = ipu_register_fb(fbi);

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
device_platform_driver(ipufb_driver);
