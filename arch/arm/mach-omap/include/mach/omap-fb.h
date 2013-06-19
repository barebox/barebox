#ifndef H_BAREBOX_ARCH_ARM_MACH_OMAP_MACH_FB4_H
#define H_BAREBOX_ARCH_ARM_MACH_OMAP_MACH_FB4_H

#include <fb.h>

#define OMAP_DSS_LCD_TFT	(1u << 0)
#define OMAP_DSS_LCD_IVS	(1u << 1)
#define OMAP_DSS_LCD_IHS	(1u << 2)
#define OMAP_DSS_LCD_IPC	(1u << 3)
#define OMAP_DSS_LCD_IEO	(1u << 4)
#define OMAP_DSS_LCD_RF	(1u << 5)
#define OMAP_DSS_LCD_ONOFF	(1u << 6)

#define OMAP_DSS_LCD_DATALINES(_l)	((_l) << 10)
#define OMAP_DSS_LCD_DATALINES_msk	OMAP_DSS_LCD_DATALINES(3u)
#define OMAP_DSS_LCD_DATALINES_12	OMAP_DSS_LCD_DATALINES(0u)
#define OMAP_DSS_LCD_DATALINES_16	OMAP_DSS_LCD_DATALINES(1u)
#define OMAP_DSS_LCD_DATALINES_18	OMAP_DSS_LCD_DATALINES(2u)
#define OMAP_DSS_LCD_DATALINES_24	OMAP_DSS_LCD_DATALINES(3u)

struct omapfb_display {
	struct fb_videomode mode;

	unsigned long config;

	unsigned int power_on_delay;
	unsigned int power_off_delay;
};

struct omapfb_platform_data {
	struct omapfb_display const *displays;
	size_t num_displays;

	unsigned int dss_clk_hz;

	unsigned int bpp;

	struct resource const *screen;

	void (*enable)(int p);
};

struct device_d;
struct device_d *omap_add_display(struct omapfb_platform_data *o_pdata);


#endif	/* H_BAREBOX_ARCH_ARM_MACH_OMAP_MACH_FB4_H */
