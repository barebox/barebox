/*
 * i.MX IPUv3 DP Overlay Planes
 *
 * Copyright (C) 2013 Philipp Zabel, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <malloc.h>
#include <fb.h>

#include "ipuv3-plane.h"
#include "imx-ipu-v3.h"

static int calc_vref(struct fb_videomode *mode)
{
	unsigned long htotal, vtotal;

	htotal = mode->left_margin + mode->right_margin + mode->xres + mode->hsync_len;
	vtotal = mode->upper_margin + mode->lower_margin + mode->yres + mode->vsync_len;

	return DIV_ROUND_UP(PICOS2KHZ(mode->pixclock) * 1000, vtotal * htotal);
}

static inline int calc_bandwidth(int width, int height, unsigned int vref)
{
	return width * height * vref;
}

int ipu_plane_set_base(struct ipu_plane *ipu_plane, struct fb_info *info,
		       int x, int y)
{
	struct ipu_ch_param __iomem *cpmem;

	pr_debug("phys = 0x%p, x = %d, y = %d, line_length = %d\n",
		info->screen_base, x, y, info->line_length);

	cpmem = ipu_get_cpmem(ipu_plane->ipu_ch);
	ipu_cpmem_set_stride(cpmem, info->line_length);
	ipu_cpmem_set_buffer(cpmem, 0, (unsigned long)info->screen_base +
			     info->line_length * y + x);
	ipu_cpmem_set_buffer(cpmem, 1, (unsigned long)info->screen_base +
			     info->line_length * y + x);

	return 0;
}

int ipu_plane_mode_set(struct ipu_plane *ipu_plane,
		       struct fb_videomode *mode,
		       struct fb_info *info, u32 pixel_format,
		       int crtc_x, int crtc_y,
		       unsigned int crtc_w, unsigned int crtc_h,
		       uint32_t src_x, uint32_t src_y,
		       uint32_t src_w, uint32_t src_h)
{
	struct ipu_ch_param __iomem *cpmem;
	struct device_d *dev = &info->dev;
	int ret;

	/* no scaling */
	if (src_w != crtc_w || src_h != crtc_h)
		return -EINVAL;

	/* clip to crtc bounds */
	if (crtc_x < 0) {
		if (-crtc_x > crtc_w)
			return -EINVAL;
		src_x += -crtc_x;
		src_w -= -crtc_x;
		crtc_w -= -crtc_x;
		crtc_x = 0;
	}
	if (crtc_y < 0) {
		if (-crtc_y > crtc_h)
			return -EINVAL;
		src_y += -crtc_y;
		src_h -= -crtc_y;
		crtc_h -= -crtc_y;
		crtc_y = 0;
	}
	if (crtc_x + crtc_w > mode->xres) {
		if (crtc_x > mode->xres)
			return -EINVAL;
		crtc_w = mode->xres - crtc_x;
		src_w = crtc_w;
	}
	if (crtc_y + crtc_h > mode->yres) {
		if (crtc_y > mode->yres)
			return -EINVAL;
		crtc_h = mode->yres - crtc_y;
		src_h = crtc_h;
	}
	/* full plane minimum width is 13 pixels */
	if (crtc_w < 13 && (ipu_plane->dp_flow != IPU_DP_FLOW_SYNC_FG))
		return -EINVAL;
	if (crtc_h < 2)
		return -EINVAL;

	switch (ipu_plane->dp_flow) {
	case IPU_DP_FLOW_SYNC_BG:
		ret = ipu_dp_setup_channel(ipu_plane->dp,
				IPUV3_COLORSPACE_RGB,
				IPUV3_COLORSPACE_RGB);
		if (ret) {
			dev_err(dev,
				"initializing display processor failed with %d\n",
				ret);
			return ret;
		}
		ipu_dp_set_global_alpha(ipu_plane->dp, 1, 0, 1);
		break;
	case IPU_DP_FLOW_SYNC_FG:
		ipu_dp_setup_channel(ipu_plane->dp,
				IPUV3_COLORSPACE_RGB,
				IPUV3_COLORSPACE_UNKNOWN);
		ipu_dp_set_window_pos(ipu_plane->dp, crtc_x, crtc_y);
		break;
	}

	ret = ipu_dmfc_init_channel(ipu_plane->dmfc, crtc_w);
	if (ret) {
		dev_err(dev, "initializing dmfc channel failed with %d\n", ret);
		return ret;
	}

	ret = ipu_dmfc_alloc_bandwidth(ipu_plane->dmfc,
			calc_bandwidth(crtc_w, crtc_h,
				       calc_vref(mode)), 64);
	if (ret) {
		dev_err(dev, "allocating dmfc bandwidth failed with %d\n", ret);
		return ret;
	}

	cpmem = ipu_get_cpmem(ipu_plane->ipu_ch);
	ipu_ch_param_zero(cpmem);
	ipu_cpmem_set_resolution(cpmem, src_w, src_h);
	ret = ipu_cpmem_set_fmt(cpmem, pixel_format);
	if (ret < 0) {
		dev_err(dev, "unsupported pixel format 0x%08x\n",
			pixel_format);
		return ret;
	}
	ipu_cpmem_set_high_priority(ipu_plane->ipu_ch);

	ret = ipu_plane_set_base(ipu_plane, info, src_x, src_y);
	if (ret < 0)
		return ret;

	return 0;
}

void ipu_plane_put_resources(struct ipu_plane *ipu_plane)
{
	if (!IS_ERR_OR_NULL(ipu_plane->dp))
		ipu_dp_put(ipu_plane->dp);
	if (!IS_ERR_OR_NULL(ipu_plane->dmfc))
		ipu_dmfc_put(ipu_plane->dmfc);
	if (!IS_ERR_OR_NULL(ipu_plane->ipu_ch))
		ipu_idmac_put(ipu_plane->ipu_ch);
}

int ipu_plane_get_resources(struct ipu_plane *ipu_plane)
{
	int ret;

	ipu_plane->ipu_ch = ipu_idmac_get(ipu_plane->ipu, ipu_plane->dma);
	if (IS_ERR(ipu_plane->ipu_ch)) {
		ret = PTR_ERR(ipu_plane->ipu_ch);
		pr_err("failed to get idmac channel: %d\n", ret);
		return ret;
	}

	ipu_plane->dmfc = ipu_dmfc_get(ipu_plane->ipu, ipu_plane->dma);
	if (IS_ERR(ipu_plane->dmfc)) {
		ret = PTR_ERR(ipu_plane->dmfc);
		pr_err("failed to get dmfc: ret %d\n", ret);
		goto err_out;
	}

	if (ipu_plane->dp_flow >= 0) {
		ipu_plane->dp = ipu_dp_get(ipu_plane->ipu, ipu_plane->dp_flow);
		if (IS_ERR(ipu_plane->dp)) {
			ret = PTR_ERR(ipu_plane->dp);
			pr_err("failed to get dp flow: %d\n", ret);
			goto err_out;
		}
	}

	return 0;
err_out:
	ipu_plane_put_resources(ipu_plane);

	return ret;
}

void ipu_plane_enable(struct ipu_plane *ipu_plane)
{
	ipu_dmfc_enable_channel(ipu_plane->dmfc);
	ipu_idmac_enable_channel(ipu_plane->ipu_ch);
	if (ipu_plane->dp)
		ipu_dp_enable_channel(ipu_plane->dp);

	ipu_plane->enabled = true;
}

void ipu_plane_disable(struct ipu_plane *ipu_plane)
{
	ipu_plane->enabled = false;

	ipu_idmac_wait_busy(ipu_plane->ipu_ch, 50);

	if (ipu_plane->dp)
		ipu_dp_disable_channel(ipu_plane->dp);
	ipu_idmac_disable_channel(ipu_plane->ipu_ch);
	ipu_dmfc_disable_channel(ipu_plane->dmfc);
}

struct ipu_plane *ipu_plane_init(struct ipu_soc *ipu,
				 int dma, int dp)
{
	struct ipu_plane *ipu_plane;

	ipu_plane = xzalloc(sizeof(*ipu_plane));

	ipu_plane->ipu = ipu;
	ipu_plane->dma = dma;
	ipu_plane->dp_flow = dp;

	return ipu_plane;
}
