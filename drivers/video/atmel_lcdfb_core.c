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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <malloc.h>
#include <asm/mmu.h>

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

static void atmel_lcdc_power_controller(struct fb_info *fb_info, int i)
{
	struct atmel_lcdfb_info *sinfo = fb_info->priv;
	struct atmel_lcdfb_platform_data *pdata = sinfo->pdata;

	if (pdata->atmel_lcdfb_power_control)
		pdata->atmel_lcdfb_power_control(1);
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
	struct atmel_lcdfb_platform_data *pdata = sinfo->pdata;
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
		/* Older SOCs use IBGR:555 rather than BGR:565. */
		if (pdata->have_intensity_bit)
			info->green.length = 5;
		else
			info->green.length = 6;
		if (pdata->lcd_wiring_mode == ATMEL_LCDC_WIRING_RGB) {
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
		if (pdata->lcd_wiring_mode == ATMEL_LCDC_WIRING_RGB) {
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

	info->screen_base = dma_alloc_coherent(smem_len);

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

int atmel_lcdc_register(struct device_d *dev, struct atmel_lcdfb_devdata *data)
{
	struct atmel_lcdfb_info *sinfo;
	struct atmel_lcdfb_platform_data *pdata = dev->platform_data;
	int ret = 0;
	struct fb_info *info;

	if (!pdata) {
		dev_err(dev, "missing platform_data\n");
		return -EINVAL;
	}

	sinfo = xzalloc(sizeof(*sinfo));
	sinfo->pdata = pdata;
	sinfo->mmio = dev_request_mem_region(dev, 0);

	sinfo->dev_data = data;

	/* just init */
	info = &sinfo->info;
	info->priv = sinfo;
	info->fbops = &atmel_lcdc_ops;
	info->mode_list = pdata->mode_list;
	info->num_modes = pdata->num_modes;
	info->mode = &info->mode_list[0];
	info->xres = info->mode->xres;
	info->yres = info->mode->yres;
	info->bits_per_pixel = pdata->default_bpp;

	/* Enable LCDC Clocks */
	sinfo->bus_clk = clk_get(dev, "hck1");
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
		sinfo->dma_desc = dma_alloc_coherent(data->dma_desc_size);

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
