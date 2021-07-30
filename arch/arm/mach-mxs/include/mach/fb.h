/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __MACH_FB_H
# define __MACH_FB_H

#include <fb.h>

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
	unsigned ld_intf_width;	/* interface width in bits */
	unsigned bits_per_pixel;

	void *fixed_screen;	/**< if != NULL use this as framebuffer memory */
	unsigned fixed_screen_size; /**< framebuffer memory size for fixed_screen */

	unsigned flags;
	void (*enable)(int enable); /**< hook to enable backlight */
};

#endif /* __MACH_FB_H */

