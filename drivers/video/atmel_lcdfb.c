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
#include <io.h>
#include <init.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <errno.h>
#include <linux/clk.h>

#include "atmel_lcdfb.h"

/* configurable parameters */
#define ATMEL_LCDC_CVAL_DEFAULT		0xc8
#define ATMEL_LCDC_DMA_BURST_LEN	8	/* words */
#define ATMEL_LCDC_FIFO_SIZE		512	/* words */

static unsigned long compute_hozval(struct atmel_lcdfb_info *sinfo,
				    unsigned long xres, unsigned long lcdcon2)
{
	unsigned long value;

	if (!sinfo->have_hozval)
		return xres;

	value = xres;
	if ((lcdcon2 & ATMEL_LCDC_DISTYPE) != ATMEL_LCDC_DISTYPE_TFT) {
		/* STN display */
		if ((lcdcon2 & ATMEL_LCDC_DISTYPE) == ATMEL_LCDC_DISTYPE_STNCOLOR)
			value *= 3;

		if ( (lcdcon2 & ATMEL_LCDC_IFWIDTH) == ATMEL_LCDC_IFWIDTH_4
		   || ( (lcdcon2 & ATMEL_LCDC_IFWIDTH) == ATMEL_LCDC_IFWIDTH_8
		      && (lcdcon2 & ATMEL_LCDC_SCANMOD) == ATMEL_LCDC_SCANMOD_DUAL ))
			value = DIV_ROUND_UP(value, 4);
		else
			value = DIV_ROUND_UP(value, 8);
	}

	return value;
}

static void atmel_lcdfb_stop(struct atmel_lcdfb_info *sinfo, u32 flags)
{
	/* Turn off the LCD controller and the DMA controller */
	lcdc_writel(sinfo, ATMEL_LCDC_PWRCON,
			sinfo->guard_time << ATMEL_LCDC_GUARDT_OFFSET);

	/* Wait for the LCDC core to become idle */
	while (lcdc_readl(sinfo, ATMEL_LCDC_PWRCON) & ATMEL_LCDC_BUSY)
		mdelay(10);

	lcdc_writel(sinfo, ATMEL_LCDC_DMACON, 0);

	if (flags & ATMEL_LCDC_STOP_NOWAIT)
		return;

	/* Wait for DMA engine to become idle... */
	while (lcdc_readl(sinfo, ATMEL_LCDC_DMACON) & ATMEL_LCDC_DMABUSY)
		mdelay(10);
}

static void atmel_lcdfb_start(struct atmel_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ATMEL_LCDC_DMACON, sinfo->dmacon);
	lcdc_writel(sinfo, ATMEL_LCDC_PWRCON,
		(sinfo->guard_time << ATMEL_LCDC_GUARDT_OFFSET)
		| ATMEL_LCDC_PWR);
}

static void atmel_lcdfb_update_dma(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;
	unsigned long dma_addr;

	dma_addr = (unsigned long)info->screen_base;

	dma_addr &= ~3UL;

	/* Set framebuffer DMA base address and pixel offset */
	lcdc_writel(sinfo, ATMEL_LCDC_DMABADDR1, dma_addr);
}

static void atmel_lcdfb_limit_screeninfo(struct fb_videomode *mode)
{
	/* Saturate vertical and horizontal timings at maximum values */
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

}

static void atmel_lcdfb_setup_core(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;
	struct fb_videomode *mode = info->mode;
	unsigned long clk_value_khz;
	unsigned long pix_factor = 2;
	unsigned long hozval_linesz;
	unsigned long value;

	/* ...set frame size and burst length = 8 words (?) */
	value = (mode->yres * mode->xres * info->bits_per_pixel) / 32;
	value |= ((ATMEL_LCDC_DMA_BURST_LEN - 1) << ATMEL_LCDC_BLENGTH_OFFSET);
	lcdc_writel(sinfo, ATMEL_LCDC_DMAFRMCFG, value);

	/* Set pixel clock */
	if (sinfo->have_alt_pixclock)
		pix_factor = 1;

	clk_value_khz = clk_get_rate(sinfo->lcdc_clk) / 1000;

	value = DIV_ROUND_UP(clk_value_khz, PICOS2KHZ(mode->pixclock));

	if (value < pix_factor) {
		dev_notice(&info->dev, "Bypassing pixel clock divider\n");
		lcdc_writel(sinfo, ATMEL_LCDC_LCDCON1, ATMEL_LCDC_BYPASS);
	} else {
		value = (value / pix_factor) - 1;
		dev_dbg(&info->dev, "  * programming CLKVAL = 0x%08lx\n",
				value);
		lcdc_writel(sinfo, ATMEL_LCDC_LCDCON1,
				value << ATMEL_LCDC_CLKVAL_OFFSET);
		mode->pixclock =
			KHZ2PICOS(clk_value_khz / (pix_factor * (value + 1)));
		dev_dbg(&info->dev, "  updated pixclk:     %lu KHz\n",
					PICOS2KHZ(mode->pixclock));
	}

	/* Initialize control register 2 */
	value = sinfo->lcdcon2;

	if (!(mode->sync & FB_SYNC_HOR_HIGH_ACT))
		value |= ATMEL_LCDC_INVLINE_INVERTED;
	if (!(mode->sync & FB_SYNC_VERT_HIGH_ACT))
		value |= ATMEL_LCDC_INVFRAME_INVERTED;

	switch (info->bits_per_pixel) {
		case 1:	value |= ATMEL_LCDC_PIXELSIZE_1; break;
		case 2: value |= ATMEL_LCDC_PIXELSIZE_2; break;
		case 4: value |= ATMEL_LCDC_PIXELSIZE_4; break;
		case 8: value |= ATMEL_LCDC_PIXELSIZE_8; break;
		case 16: value |= ATMEL_LCDC_PIXELSIZE_16; break;
		case 24: value |= ATMEL_LCDC_PIXELSIZE_24; break;
		case 32: value |= ATMEL_LCDC_PIXELSIZE_32; break;
		default: BUG(); break;
	}
	dev_dbg(&info->dev, "  * LCDCON2 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCON2, value);

	/* Vertical timing */
	value = (mode->vsync_len - 1) << ATMEL_LCDC_VPW_OFFSET;
	value |= mode->upper_margin << ATMEL_LCDC_VBP_OFFSET;
	value |= mode->lower_margin;
	dev_dbg(&info->dev, "  * LCDTIM1 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_TIM1, value);

	/* Horizontal timing */
	value = (mode->right_margin - 1) << ATMEL_LCDC_HFP_OFFSET;
	value |= (mode->hsync_len - 1) << ATMEL_LCDC_HPW_OFFSET;
	value |= (mode->left_margin - 1);
	dev_dbg(&info->dev, "  * LCDTIM2 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_TIM2, value);

	/* Horizontal value (aka line size) */
	hozval_linesz = compute_hozval(sinfo, mode->xres,
					lcdc_readl(sinfo, ATMEL_LCDC_LCDCON2));

	/* Display size */
	value = (hozval_linesz - 1) << ATMEL_LCDC_HOZVAL_OFFSET;
	value |= mode->yres - 1;
	dev_dbg(&info->dev, "  * LCDFRMCFG = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDFRMCFG, value);

	/* FIFO Threshold: Use formula from data sheet */
	value = ATMEL_LCDC_FIFO_SIZE - (2 * ATMEL_LCDC_DMA_BURST_LEN + 3);
	lcdc_writel(sinfo, ATMEL_LCDC_FIFO, value);

	/* Toggle LCD_MODE every frame */
	lcdc_writel(sinfo, ATMEL_LCDC_MVAL, 0);

	/* Disable all interrupts */
	lcdc_writel(sinfo, ATMEL_LCDC_IDR, ~0UL);

	/* Enable FIFO & DMA errors */
	lcdc_writel(sinfo, ATMEL_LCDC_IER, ATMEL_LCDC_UFLWI | ATMEL_LCDC_OWRI | ATMEL_LCDC_MERI);

	/* ...wait for DMA engine to become idle... */
	while (lcdc_readl(sinfo, ATMEL_LCDC_DMACON) & ATMEL_LCDC_DMABUSY)
		mdelay(10);
}

static void atmel_lcdfb_init_contrast(struct atmel_lcdfb_info *sinfo)
{
	unsigned long value;

	value = ATMEL_LCDC_PS_DIV8 |
		ATMEL_LCDC_POL_POSITIVE |
		ATMEL_LCDC_ENA_PWMENABLE;
	lcdc_writel(sinfo, ATMEL_LCDC_CONTRAST_CTR, value);
	lcdc_writel(sinfo, ATMEL_LCDC_CONTRAST_VAL, ATMEL_LCDC_CVAL_DEFAULT);
}

struct atmel_lcdfb_devdata atmel_lcdfb_data = {
	.start = atmel_lcdfb_start,
	.stop = atmel_lcdfb_stop,
	.update_dma = atmel_lcdfb_update_dma,
	.setup_core = atmel_lcdfb_setup_core,
	.init_contrast = atmel_lcdfb_init_contrast,
	.limit_screeninfo = atmel_lcdfb_limit_screeninfo,
};

static int atmel_lcdc_probe(struct device_d *dev)
{
	return atmel_lcdc_register(dev, &atmel_lcdfb_data);
}

static struct atmel_lcdfb_config at91sam9261_config = {
	.have_hozval		= true,
	.have_intensity_bit	= true,
};

static struct atmel_lcdfb_config at91sam9263_config = {
	.have_intensity_bit	= true,
};

static struct atmel_lcdfb_config at91sam9g10_config = {
	.have_hozval		= true,
};

static struct atmel_lcdfb_config at91sam9g45_config = {
	.have_alt_pixclock	= true,
};

static struct atmel_lcdfb_config at91sam9rl_config = {
	.have_intensity_bit	= true,
};

static struct atmel_lcdfb_config at32ap_config = {
	.have_hozval		= true,
};

static __maybe_unused struct of_device_id atmel_lcdfb_compatible[] = {
	{ .compatible = "atmel,at91sam9261-lcdc",	.data = &at91sam9261_config, },
	{ .compatible = "atmel,at91sam9263-lcdc",	.data = &at91sam9263_config, },
	{ .compatible = "atmel,at91sam9g10-lcdc",	.data = &at91sam9g10_config, },
	{ .compatible = "atmel,at91sam9g45-lcdc",	.data = &at91sam9g45_config, },
	{ .compatible = "atmel,at91sam9g45es-lcdc", },
	{ .compatible = "atmel,at91sam9rl-lcdc",	.data = &at91sam9rl_config, },
	{ .compatible = "atmel,at32ap-lcdc",		.data = &at32ap_config, },
	{ /* sentinel */ }
};

static struct driver_d atmel_lcdc_driver = {
	.name	= "atmel_lcdfb",
	.probe	= atmel_lcdc_probe,
	.of_compatible = DRV_OF_COMPAT(atmel_lcdfb_compatible),
};
device_platform_driver(atmel_lcdc_driver);
