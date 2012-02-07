/*
 * Copyright (C) 2008
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MACH_IMX_IPU_FB_H__
#define __MACH_IMX_IPU_FB_H__

#include <fb.h>

/* Proprietary FB_SYNC_ flags */
#define FB_SYNC_OE_ACT_HIGH	0x80000000
#define FB_SYNC_CLK_INVERT	0x40000000
#define FB_SYNC_DATA_INVERT	0x20000000
#define FB_SYNC_CLK_IDLE_EN	0x10000000
#define FB_SYNC_SHARP_MODE	0x08000000
#define FB_SYNC_SWAP_RGB	0x04000000
#define FB_SYNC_CLK_SEL_EN	0x02000000

/*
 * struct mx3fb_platform_data - mx3fb platform data
 */
struct imx_ipu_fb_platform_data {
	struct fb_videomode	*mode;
	unsigned char		bpp;
	u_int			num_modes;
	void __iomem		*framebuffer;
	void __iomem		*framebuffer_ovl;
	/** hook to enable backlight and stuff */
	void			(*enable)(int enable);
};

#endif /* __MACH_IMX_IPU_FB_H__ */

