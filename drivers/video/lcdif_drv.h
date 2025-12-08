/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022 Marek Vasut <marex@denx.de>
 *
 * i.MX8MP/i.MXRT LCDIFv3 LCD controller driver.
 */

#ifndef __LCDIF_DRV_H__
#define __LCDIF_DRV_H__

#include <video/vpl.h>

struct clk;

struct lcdif_drm_private {
	void __iomem			*base;	/* registers */

	int				id;

	u32				line_length;
	u32				max_yres;
	int				crtc_endpoint_id;
	struct device_node		*port;
	struct fb_info			info;

	dma_addr_t			paddr;

	struct clk			*clk;
	struct clk			*clk_axi;
	struct clk			*clk_disp_axi;

	struct device			*dev;
	struct vpl			vpl;
	struct fb_videomode		*mode;
};

int lcdif_kms_init(struct lcdif_drm_private *lcdif);

#endif /* __LCDIF_DRV_H__ */
