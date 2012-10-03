/*
 * Copyright (C) 2010 Juergen Beisert
 * Copyright (C) 2011 Alexey Galakhov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#ifndef __MACH_FB_H_
# define __MACH_FB_H_

#include <fb.h>

/** Proprietary flags corresponding to  S3C24x0 LCDCON5 register */

/** ! INVVDEN - DE active high */
#define FB_SYNC_DE_HIGH_ACT	(1 << 23)
/** INVVCLK - invert CLK signal */
#define FB_SYNC_CLK_INVERT	(1 << 24)
/** INVVD - invert data */
#define FB_SYNC_DATA_INVERT	(1 << 25)
/** INVPWREN - use PWREN signal */
#define FB_SYNC_INVERT_PWREN	(1 << 26)
/** INVLEND - use LEND signal */
#define FB_SYNC_INVERT_LEND	(1 << 27)
/** PWREN - use PWREN signal */
#define FB_SYNC_USE_PWREN	(1 << 28)
/** ENLEND - use LEND signal */
#define FB_SYNC_USE_LEND	(1 << 29)
/** BSWP - swap bytes */
#define FB_SYNC_SWAP_BYTES	(1 << 30)
/** HWSWP - swap half words */
#define FB_SYNC_SWAP_HW		(1 << 31)

struct s3c_fb_platform_data {
	struct fb_videomode *mode_list;
	unsigned mode_cnt;

	unsigned bits_per_pixel;
	int passive_display;	/**< enable support for STN or CSTN displays */

	/** hook to enable backlight and stuff */
	void (*enable)(int enable);
};

#endif /* __MACH_FB_H_ */
