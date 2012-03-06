/*
 * This structure describes the machine which we are running on.
 */
#ifndef _PXAFB_
#define _PXAFB_

#include <fb.h>

/*
 * Supported LCD connections
 *
 * bits 0 - 3: for LCD panel type:
 *
 *   STN  - for passive matrix
 *   DSTN - for dual scan passive matrix
 *   TFT  - for active matrix
 *
 * bits 4 - 9 : for bus width
 * bits 10-17 : for AC Bias Pin Frequency
 * bit     18 : for output enable polarity
 * bit     19 : for pixel clock edge
 * bit     20 : for output pixel format when base is RGBT16
 */
#define LCD_CONN_TYPE(_x)	((_x) & 0x0f)
#define LCD_CONN_WIDTH(_x)	(((_x) >> 4) & 0x1f)

#define LCD_TYPE_MASK		0xf
#define LCD_TYPE_UNKNOWN	0
#define LCD_TYPE_MONO_STN	1
#define LCD_TYPE_MONO_DSTN	2
#define LCD_TYPE_COLOR_STN	3
#define LCD_TYPE_COLOR_DSTN	4
#define LCD_TYPE_COLOR_TFT	5
#define LCD_TYPE_SMART_PANEL	6
#define LCD_TYPE_MAX		7

#define LCD_MONO_STN_4BPP	((4  << 4) | LCD_TYPE_MONO_STN)
#define LCD_MONO_STN_8BPP	((8  << 4) | LCD_TYPE_MONO_STN)
#define LCD_MONO_DSTN_8BPP	((8  << 4) | LCD_TYPE_MONO_DSTN)
#define LCD_COLOR_STN_8BPP	((8  << 4) | LCD_TYPE_COLOR_STN)
#define LCD_COLOR_DSTN_16BPP	((16 << 4) | LCD_TYPE_COLOR_DSTN)
#define LCD_COLOR_TFT_8BPP	((8  << 4) | LCD_TYPE_COLOR_TFT)
#define LCD_COLOR_TFT_16BPP	((16 << 4) | LCD_TYPE_COLOR_TFT)
#define LCD_COLOR_TFT_18BPP	((18 << 4) | LCD_TYPE_COLOR_TFT)
#define LCD_SMART_PANEL_8BPP	((8  << 4) | LCD_TYPE_SMART_PANEL)
#define LCD_SMART_PANEL_16BPP	((16 << 4) | LCD_TYPE_SMART_PANEL)
#define LCD_SMART_PANEL_18BPP	((18 << 4) | LCD_TYPE_SMART_PANEL)

#define LCD_AC_BIAS_FREQ(x)	(((x) & 0xff) << 10)
#define LCD_BIAS_ACTIVE_HIGH	(0 << 18)
#define LCD_BIAS_ACTIVE_LOW	(1 << 18)
#define LCD_PCLK_EDGE_RISE	(0 << 19)
#define LCD_PCLK_EDGE_FALL	(1 << 19)
#define LCD_ALTERNATE_MAPPING	(1 << 20)

struct pxafb_videomode {
	struct fb_videomode	mode;
	u8			bpp;
};

/**
 * Define relevant framebuffer information
 */
struct pxafb_platform_data {
	struct pxafb_videomode	*mode;
	unsigned int		lcd_conn;

	/** force a memory area to be used, else NULL for dynamic allocation */
	void			*framebuffer;

	void			(*lcd_power)(int);
	void			(*backlight_power)(int);
};

/**
 * @file
 * @brief PXA related framebuffer declarations
 */
#endif
