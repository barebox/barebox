/*
 * Driver for AT91/AT32 LCD Controller
 *
 * Copyright (C) 2007 Atmel Corporation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <of_gpio.h>
#include <gpio.h>
#include <dma.h>
#include <io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <malloc.h>

#include <mach/cpu.h>

#include "atmel_lcdfb.h"

static void atmel_lcdfb_start_clock(struct atmel_lcdfb_info *sinfo)
{
	clk_enable(sinfo->bus_clk);
	clk_enable(sinfo->lcdc_clk);
}

static void atmel_lcdfb_stop_clock(struct atmel_lcdfb_info *sinfo)
{
	clk_disable(sinfo->bus_clk);
	clk_disable(sinfo->lcdc_clk);
}

static void atmel_lcdc_power_controller(struct fb_info *fb_info, int on)
{
	struct atmel_lcdfb_info *sinfo = fb_info->priv;

	if (sinfo->gpio_power_control < 0)
		return;

	if (sinfo->gpio_power_control_active_low)
		gpio_set_value(sinfo->gpio_power_control, !on);
	else
		gpio_set_value(sinfo->gpio_power_control, on);
}

/**
 * @param fb_info Framebuffer information
 */
static void atmel_lcdc_enable_controller(struct fb_info *fb_info)
{
	atmel_lcdc_power_controller(fb_info, 1);
}

/**
 * @param fb_info Framebuffer information
 */
static void atmel_lcdc_disable_controller(struct fb_info *fb_info)
{
	atmel_lcdc_power_controller(fb_info, 0);
}


static int atmel_lcdfb_check_var(struct fb_info *info)
{
	struct device_d *dev = &info->dev;
	struct atmel_lcdfb_info *sinfo = info->priv;
	struct fb_videomode *mode = info->mode;
	unsigned long clk_value_khz;

	clk_value_khz = clk_get_rate(sinfo->lcdc_clk) / 1000;

	dev_dbg(dev, "%s:\n", __func__);

	if (!(mode->pixclock && info->bits_per_pixel)) {
		dev_err(dev, "needed value not specified\n");
		return -EINVAL;
	}

	dev_dbg(dev, "  resolution: %ux%u\n", mode->xres, mode->yres);
	dev_dbg(dev, "  pixclk:     %lu KHz\n", PICOS2KHZ(mode->pixclock));
	dev_dbg(dev, "  bpp:        %u\n", info->bits_per_pixel);
	dev_dbg(dev, "  clk:        %lu KHz\n", clk_value_khz);

	if (PICOS2KHZ(mode->pixclock) > clk_value_khz) {
		dev_err(dev, "%lu KHz pixel clock is too fast\n", PICOS2KHZ(mode->pixclock));
		return -EINVAL;
	}

	/* Saturate vertical and horizontal timings at maximum values */
	if (sinfo->dev_data->limit_screeninfo)
		sinfo->dev_data->limit_screeninfo(mode);

	mode->vsync_len = min_t(u32, mode->vsync_len,
			(ATMEL_LCDC_VPW >> ATMEL_LCDC_VPW_OFFSET) + 1);
	mode->upper_margin = min_t(u32, mode->upper_margin,
			ATMEL_LCDC_VBP >> ATMEL_LCDC_VBP_OFFSET);
	mode->lower_margin = min_t(u32, mode->lower_margin,
			ATMEL_LCDC_VFP);
	mode->right_margin = min_t(u32, mode->right_margin,
			(ATMEL_LCDC_HFP >> ATMEL_LCDC_HFP_OFFSET) + 1);
	mode->hsync_len = min_t(u32, mode->hsync_len,
			(ATMEL_LCDC_HPW >> ATMEL_LCDC_HPW_OFFSET) + 1);
	mode->left_margin = min_t(u32, mode->left_margin,
			ATMEL_LCDC_HBP + 1);

	/* Some parameters can't be zero */
	mode->vsync_len = max_t(u32, mode->vsync_len, 1);
	mode->right_margin = max_t(u32, mode->right_margin, 1);
	mode->hsync_len = max_t(u32, mode->hsync_len, 1);
	mode->left_margin = max_t(u32, mode->left_margin, 1);

	switch (info->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
		info->red.offset = info->green.offset = info->blue.offset = 0;
		info->red.length = info->green.length = info->blue.length
			= info->bits_per_pixel;
		break;
	case 16:
		/* Older SOCs use BGR:555 rather than BGR:565. */
		if (sinfo->have_intensity_bit)
			info->green.length = 5;
		else
			info->green.length = 6;
		if (sinfo->lcd_wiring_mode == ATMEL_LCDC_WIRING_RGB) {
			/* RGB:5X5 mode */
			info->red.offset = info->green.length + 5;
			info->blue.offset = 0;
		} else {
			/* BGR:5X5 mode */
			info->red.offset = 0;
			info->blue.offset = info->green.length + 5;
		}
		info->green.offset = 5;
		info->red.length = info->blue.length = 5;
		break;
	case 32:
		info->transp.offset = 24;
		info->transp.length = 8;
		/* fall through */
	case 24:
		if (sinfo->lcd_wiring_mode == ATMEL_LCDC_WIRING_RGB) {
			/* RGB:888 mode */
			info->red.offset = 16;
			info->blue.offset = 0;
		} else {
			/* BGR:888 mode */
			info->red.offset = 0;
			info->blue.offset = 16;
		}
		info->green.offset = 8;
		info->red.length = info->green.length = info->blue.length = 8;
		break;
	default:
		dev_err(dev, "color depth %d not supported\n",
					info->bits_per_pixel);
		return -EINVAL;
	}

	return 0;
}

static void atmel_lcdfb_set_par(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;

	if (sinfo->dev_data->stop)
		sinfo->dev_data->stop(sinfo, ATMEL_LCDC_STOP_NOWAIT);

	/* Re-initialize the DMA engine... */
	dev_dbg(&info->dev, "  * update DMA engine\n");
	sinfo->dev_data->update_dma(info);

	/* Now, the LCDC core... */
	sinfo->dev_data->setup_core(info);

	if (sinfo->dev_data->start)
		sinfo->dev_data->start(sinfo);

	dev_dbg(&info->dev, "  * DONE\n");
}

static int atmel_lcdfb_alloc_video_memory(struct atmel_lcdfb_info *sinfo)
{
	struct fb_info *info = &sinfo->info;
	struct fb_videomode *mode = info->mode;
	unsigned int smem_len;

	free(info->screen_base);

	smem_len = (mode->xres * mode->yres
		    * ((info->bits_per_pixel + 7) / 8));
	smem_len = max(smem_len, sinfo->smem_len);

	info->screen_base = dma_alloc_coherent(smem_len, DMA_ADDRESS_BROKEN);

	if (!info->screen_base)
		return -ENOMEM;

	memset(info->screen_base, 0, smem_len);

	return 0;
}

/**
 * Prepare the video hardware for a specified video mode
 * @param fb_info Framebuffer information
 * @param mode The video mode description to initialize
 * @return 0 on success
 */
static int atmel_lcdc_activate_var(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;
	int ret;

	ret = atmel_lcdfb_alloc_video_memory(sinfo);
	if (ret)
		return ret;

	atmel_lcdfb_set_par(info);

	if (sinfo->dev_data->init_contrast)
		sinfo->dev_data->init_contrast(sinfo);

	return atmel_lcdfb_check_var(info);
}

/*
 * There is only one video hardware instance available.
 * It makes no sense to dynamically allocate this data
 */
static struct fb_ops atmel_lcdc_ops = {
	.fb_activate_var = atmel_lcdc_activate_var,
	.fb_enable = atmel_lcdc_enable_controller,
	.fb_disable = atmel_lcdc_disable_controller,
};

static int power_control_init(struct device_d *dev,
			      struct atmel_lcdfb_info *sinfo,
			      int gpio,
			      bool active_low)
{
	int ret;
	const char *name = "lcdc_power";

	sinfo->gpio_power_control = gpio;
	sinfo->gpio_power_control_active_low = active_low;

	/* If no GPIO specified then stop */
	if (!gpio_is_valid(gpio))
		return 0;

	ret = gpio_request(gpio, name);
	if (ret) {
		dev_err(dev, "%s: can not request gpio %d (%d)\n",
			name, gpio, ret);
		return ret;
	}
	ret = gpio_direction_output(gpio, 1);
	if (ret) {
		dev_err(dev, "%s: can not configure gpio %d as output (%d)\n",
			name, gpio, ret);
		return ret;
	}

	return ret;
}

/*
 * Syntax: atmel,lcd-wiring-mode: lcd wiring mode "RGB", "BGR"
 */
static int of_get_wiring_mode(struct device_node *np,
			      struct atmel_lcdfb_info *sinfo)
{
	const char *mode;
	int ret;

	ret = of_property_read_string(np, "atmel,lcd-wiring-mode", &mode);
	if (ret < 0) {
		/* Not present, use defaults */
		sinfo->lcd_wiring_mode = ATMEL_LCDC_WIRING_BGR;
		return 0;
	}

	if (!strcasecmp(mode, "BGR")) {
		sinfo->lcd_wiring_mode = ATMEL_LCDC_WIRING_BGR;
	} else if (!strcasecmp(mode, "RGB")) {
		sinfo->lcd_wiring_mode = ATMEL_LCDC_WIRING_RGB;
	} else {
		return -ENODEV;
	}
	return 0;
}

static int of_get_power_control(struct device_d *dev,
				struct device_node *np,
				struct atmel_lcdfb_info *sinfo)
{
	enum of_gpio_flags flags;
	bool active_low;
	int gpio;

	gpio = of_get_named_gpio_flags(np, "atmel,power-control-gpio", 0, &flags);
	if (!gpio_is_valid(gpio)) {
		/* No power control - ignore */
		return 0;
	}
	active_low = (flags & OF_GPIO_ACTIVE_LOW ? true : false);
	return power_control_init(dev, sinfo, gpio, active_low);
}

static int lcdfb_of_init(struct device_d *dev, struct atmel_lcdfb_info *sinfo)
{
	struct fb_info *info = &sinfo->info;
	struct display_timings *modes;
	struct device_node *display;
	struct atmel_lcdfb_config *config;
	int ret;

	/* Driver data - optional */
	ret = dev_get_drvdata(dev, (const void **)&config);
	if (!ret) {
		sinfo->have_hozval = config->have_hozval;
		sinfo->have_intensity_bit = config->have_intensity_bit;
		sinfo->have_alt_pixclock = config->have_alt_pixclock;
	}

	/* Required properties */
	display = of_parse_phandle(dev->device_node, "display", 0);
	if (!display) {
		dev_err(dev, "no display phandle\n");
		return -ENOENT;
	}
	ret = of_property_read_u32(display, "atmel,guard-time", &sinfo->guard_time);
	if (ret < 0) {
		dev_err(dev, "failed to get atmel,guard-time property\n");
		goto err;
	}
	ret = of_property_read_u32(display, "atmel,lcdcon2", &sinfo->lcdcon2);
	if (ret < 0) {
		dev_err(dev, "failed to get atmel,lcdcon2 property\n");
		goto err;
	}
	ret = of_property_read_u32(display, "atmel,dmacon", &sinfo->dmacon);
	if (ret < 0) {
		dev_err(dev, "failed to get atmel,dmacon property\n");
		goto err;
	}
	ret = of_property_read_u32(display, "bits-per-pixel", &info->bits_per_pixel);
	if (ret < 0) {
		dev_err(dev, "failed to get bits-per-pixel property\n");
		goto err;
	}
	modes = of_get_display_timings(display);
	if (IS_ERR(modes)) {
		dev_err(dev, "unable to parse display timings\n");
		ret = PTR_ERR(modes);
		goto err;
	}
	info->modes.modes = modes->modes;
	info->modes.num_modes = modes->num_modes;

	/* Optional properties */
	ret = of_get_wiring_mode(display, sinfo);
	if (ret < 0) {
		dev_err(dev, "failed to get atmel,lcd-wiring-mode property\n");
		goto err;
	}
	ret = of_get_power_control(dev, display, sinfo);
	if (ret < 0) {
		dev_err(dev, "failed to get power control gpio\n");
		goto err;
	}
	return 0;
err:
	return ret;
}

static int lcdfb_pdata_init(struct device_d *dev, struct atmel_lcdfb_info *sinfo)
{
	struct atmel_lcdfb_platform_data *pdata;
	struct fb_info *info;
	bool active_low;
	int gpio;
	int ret;

	pdata = dev->platform_data;

	/* If gpio == 0 (default in pdata) then we assume no power control */
	gpio = pdata->gpio_power_control;
	if (gpio == 0)
		gpio = -1;

	active_low = pdata->gpio_power_control_active_low;
	ret = power_control_init(dev, sinfo, gpio, active_low);
	if (ret)
		goto err;

	sinfo->guard_time = pdata->guard_time;
	sinfo->lcdcon2 = pdata->default_lcdcon2;
	sinfo->dmacon = pdata->default_dmacon;
	sinfo->lcd_wiring_mode = pdata->lcd_wiring_mode;

	sinfo->have_alt_pixclock = cpu_is_at91sam9g45() &&
				   !cpu_is_at91sam9g45es();
	sinfo->have_intensity_bit = pdata->have_intensity_bit;
	sinfo->have_hozval = cpu_is_at91sam9261() ||
			     cpu_is_at91sam9g10() ||
			     cpu_is_at32ap7000();

	info = &sinfo->info;
	info->modes.modes = pdata->mode_list;
	info->modes.num_modes = pdata->num_modes;
	info->mode = &info->modes.modes[0];
	info->xres = info->mode->xres;
	info->yres = info->mode->yres;
	info->bits_per_pixel = pdata->default_bpp;

err:
	return ret;
}

int atmel_lcdc_register(struct device_d *dev, struct atmel_lcdfb_devdata *data)
{
	struct atmel_lcdfb_info *sinfo;
	const char *bus_clk_name;
	struct resource *iores;
	struct fb_info *info;
	int ret = 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	sinfo = xzalloc(sizeof(*sinfo));
	sinfo->dev_data = data;
	sinfo->mmio = IOMEM(iores->start);

	info = &sinfo->info;
	info->priv = sinfo;
	info->fbops = &atmel_lcdc_ops;

	if (dev->platform_data) {
		ret = lcdfb_pdata_init(dev, sinfo);
		if (ret) {
			dev_err(dev, "failed to init lcdfb from pdata\n");
			goto err;
		}
		bus_clk_name = "hck1";
	} else {
		if (!IS_ENABLED(CONFIG_OFDEVICE) || !dev->device_node)
			return -EINVAL;

		ret = lcdfb_of_init(dev, sinfo);
		if (ret) {
			dev_err(dev, "failed to init lcdfb from DT\n");
			goto err;
		}
		bus_clk_name = "hclk";
	}

	/* Enable LCDC Clocks */
	sinfo->bus_clk = clk_get(dev, bus_clk_name);
	if (IS_ERR(sinfo->bus_clk)) {
		ret = PTR_ERR(sinfo->bus_clk);
		goto err;
	}
	sinfo->lcdc_clk = clk_get(dev, "lcdc_clk");
	if (IS_ERR(sinfo->lcdc_clk)) {
		ret = PTR_ERR(sinfo->lcdc_clk);
		goto put_bus_clk;
	}

	atmel_lcdfb_start_clock(sinfo);

	if (data->dma_desc_size)
		sinfo->dma_desc = dma_alloc_coherent(data->dma_desc_size,
						     DMA_ADDRESS_BROKEN);

	ret = register_framebuffer(info);
	if (ret != 0) {
		dev_err(dev, "Failed to register framebuffer\n");
		goto stop_clk;
	}

	return ret;

stop_clk:
	if (sinfo->dma_desc)
		free(sinfo->dma_desc);
	atmel_lcdfb_stop_clock(sinfo);
	clk_put(sinfo->lcdc_clk);
put_bus_clk:
	clk_put(sinfo->bus_clk);
err:
	return ret;
}
