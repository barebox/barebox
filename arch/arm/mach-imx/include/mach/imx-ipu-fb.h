/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2008 Guennadi Liakhovetski <lg@denx.de>, DENX Software Engineering */

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
 * Specify the way your display is connected. The IPU can arbitrarily
 * map the internal colors to the external data lines. We only support
 * the following mappings at the moment.
 */
enum disp_data_mapping {
	/* blue -> d[0..5], green -> d[6..11], red -> d[12..17] */
	IPU_DISP_DATA_MAPPING_RGB666,
	/* blue -> d[0..4], green -> d[5..10], red -> d[11..15] */
	IPU_DISP_DATA_MAPPING_RGB565,
	/* blue -> d[0..7], green -> d[8..15], red -> d[16..23] */
	IPU_DISP_DATA_MAPPING_RGB888,
};

/*
 * struct mx3fb_platform_data - mx3fb platform data
 */
struct imx_ipu_fb_platform_data {
	struct fb_videomode	*mode;
	unsigned char		bpp;
	u_int			num_modes;
	enum disp_data_mapping	disp_data_fmt;
	void __iomem		*framebuffer;
	unsigned long		framebuffer_size;
	void __iomem		*framebuffer_ovl;
	unsigned long		framebuffer_ovl_size;
	/** hook to enable backlight and stuff */
	void			(*enable)(int enable);
	/*
	 * Fractional pixelclock divider causes jitter which some displays
	 * or LVDS transceivers can't handle. Disable it if necessary.
	 */
	int			disable_fractional_divider;
};

#endif /* __MACH_IMX_IPU_FB_H__ */

