/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_FB_H
# define __MACH_FB_H

#include <fb.h>

#define STMLCDIF_8BIT 1	/** pixel data bus to the display is of 8 bit width */
#define STMLCDIF_16BIT 0 /** pixel data bus to the display is of 16 bit width */
#define STMLCDIF_18BIT 2 /** pixel data bus to the display is of 18 bit width */
#define STMLCDIF_24BIT 3 /** pixel data bus to the display is of 24 bit width */

/** LC display uses active high data enable signal */
#define FB_SYNC_DE_HIGH_ACT	(1 << 27)
/** LC display will latch its data at clock's rising edge */
#define FB_SYNC_CLK_INVERT	(1 << 28)
/** output RGB digital data inverted */
#define FB_SYNC_DATA_INVERT	(1 << 29)
/** Stop clock if no data is sent (required for passive displays) */
#define FB_SYNC_CLK_IDLE_DIS	(1 << 30)
/** swap RGB to BGR */
#define FB_SYNC_SWAP_RGB	(1 << 31)

#define USE_LCD_RESET		1

struct imx_fb_platformdata {
	struct fb_videomode *mode_list;
	unsigned mode_cnt;

	unsigned dotclk_delay;	/**< refer manual HW_LCDIF_VDCTRL4 register */
	unsigned ld_intf_width;	/**< refer STMLCDIF_* macros */
	unsigned bits_per_pixel;

	void *fixed_screen;	/**< if != NULL use this as framebuffer memory */
	unsigned fixed_screen_size; /**< framebuffer memory size for fixed_screen */

	unsigned flags;
	void (*enable)(int enable); /**< hook to enable backlight */
};

#endif /* __MACH_FB_H */

