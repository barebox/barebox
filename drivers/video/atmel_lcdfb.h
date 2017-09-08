
#include <fb.h>
#include <video/atmel_lcdc.h>

struct atmel_lcdfb_info;

struct atmel_lcdfb_devdata {
	void (*start)(struct atmel_lcdfb_info *sinfo);
	void (*stop)(struct atmel_lcdfb_info *sinfo, u32 flags);
	void (*update_dma)(struct fb_info *info);
	void (*setup_core)(struct fb_info *info);
	void (*init_contrast)(struct atmel_lcdfb_info *sinfo);
	void (*limit_screeninfo)(struct fb_videomode *mode);
	int dma_desc_size;
};

struct atmel_lcdfb_info {
	struct fb_info		info;
	void __iomem		*mmio;
	struct device_d		*device;

	unsigned int		guard_time;
	unsigned int		smem_len;
	unsigned int		lcdcon2;
	unsigned int		dmacon;
	unsigned int		lcd_wiring_mode;
	bool			have_intensity_bit;

	int			gpio_power_control;
	bool			gpio_power_control_active_low;
	struct clk		*bus_clk;
	struct clk		*lcdc_clk;

	struct atmel_lcdfb_devdata *dev_data;
	void			*dma_desc;
};

#define lcdc_readl(sinfo, reg)		__raw_readl((sinfo)->mmio+(reg))
#define lcdc_writel(sinfo, reg, val)	__raw_writel((val), (sinfo)->mmio+(reg))

#define ATMEL_LCDC_STOP_NOWAIT (1 << 0)

int atmel_lcdc_register(struct device_d *dev, struct atmel_lcdfb_devdata *data);
