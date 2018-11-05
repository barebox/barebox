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
#include <linux/clk.h>
#include <mach/hardware.h>
#include <mach/atmel_hlcdc.h>
#include <mach/cpu.h>
#include <errno.h>

#include "atmel_lcdfb.h"

#define ATMEL_LCDC_CVAL_DEFAULT		0xc8

static void atmel_hlcdfb_stop(struct atmel_lcdfb_info *sinfo, u32 flags)
{
	/* Disable DISP signal */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDDIS, LCDC_LCDDIS_DISPDIS);
	while ((lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_DISPSTS))
		mdelay(1);
	/* Disable synchronization */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDDIS, LCDC_LCDDIS_SYNCDIS);
	while ((lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_LCDSTS))
		mdelay(1);
	/* Disable pixel clock */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDDIS, LCDC_LCDDIS_CLKDIS);
	while ((lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_CLKSTS))
		mdelay(1);
	/* Disable PWM */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDDIS, LCDC_LCDDIS_PWMDIS);
	while ((lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_PWMSTS))
		mdelay(1);

	if (!(flags & ATMEL_LCDC_STOP_NOWAIT))
		/* Wait for the end of DMA transfer */
		while (!(lcdc_readl(sinfo, ATMEL_LCDC_BASEISR) & LCDC_BASEISR_DMA))
			mdelay(10);
		/*FIXME: OVL DMA? */
}

static void atmel_hlcdfb_start(struct atmel_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ATMEL_LCDC_LCDEN, LCDC_LCDEN_CLKEN);
	while (!(lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_CLKSTS))
		mdelay(1);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDEN, LCDC_LCDEN_SYNCEN);
	while (!(lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_LCDSTS))
		mdelay(1);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDEN, LCDC_LCDEN_DISPEN);
	while (!(lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_DISPSTS))
		mdelay(1);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDEN, LCDC_LCDEN_PWMEN);
	while (!(lcdc_readl(sinfo, ATMEL_LCDC_LCDSR) & LCDC_LCDSR_PWMSTS))
		mdelay(1);
}

struct atmel_hlcd_dma_desc {
	u32	address;
	u32	control;
	u32	next;
};

static void atmel_hlcdfb_update_dma(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;
	unsigned long dma_addr;
	struct atmel_hlcd_dma_desc *desc;

	dma_addr = (u32)info->screen_base;
	dma_addr &= ~3UL;

	/* Setup the DMA descriptor, this descriptor will loop to itself */
	desc = sinfo->dma_desc;

	desc->address = dma_addr;
	/* Disable DMA transfer interrupt & descriptor loaded interrupt. */
	desc->control = LCDC_BASECTRL_ADDIEN | LCDC_BASECTRL_DSCRIEN
			| LCDC_BASECTRL_DMAIEN | LCDC_BASECTRL_DFETCH;
	desc->next = (u32)desc;

	lcdc_writel(sinfo, ATMEL_LCDC_BASEADDR, desc->address);
	lcdc_writel(sinfo, ATMEL_LCDC_BASECTRL, desc->control);
	lcdc_writel(sinfo, ATMEL_LCDC_BASENEXT, desc->next);
	lcdc_writel(sinfo, ATMEL_LCDC_BASECHER, LCDC_BASECHER_CHEN | LCDC_BASECHER_UPDATEEN);
}

static void atmel_hlcdfb_limit_screeninfo(struct fb_videomode *mode)
{
	u32 hbpw, hfpw;

	if (cpu_is_at91sam9x5()) {
		hbpw = LCDC_LCDCFG3_HBPW;
		hfpw = LCDC_LCDCFG3_HFPW;
	} else {
		hbpw = LCDC2_LCDCFG3_HBPW;
		hfpw = LCDC2_LCDCFG3_HFPW;
	}

	/* Saturate vertical and horizontal timings at maximum values */
	mode->vsync_len = min_t(u32, mode->vsync_len,
			(LCDC_LCDCFG1_VSPW >> LCDC_LCDCFG1_VSPW_OFFSET) + 1);
	mode->upper_margin = min_t(u32, mode->upper_margin,
			(LCDC_LCDCFG2_VFPW >> LCDC_LCDCFG2_VFPW_OFFSET) + 1);
	mode->lower_margin = min_t(u32, mode->lower_margin,
			LCDC_LCDCFG2_VBPW >> LCDC_LCDCFG2_VBPW_OFFSET);
	mode->right_margin = min_t(u32, mode->right_margin,
			(hbpw >> LCDC_LCDCFG3_HBPW_OFFSET) + 1);
	mode->hsync_len = min_t(u32, mode->hsync_len,
			(LCDC_LCDCFG1_HSPW >> LCDC_LCDCFG1_HSPW_OFFSET) + 1);
	mode->left_margin = min_t(u32, mode->left_margin,
			(hfpw >> LCDC_LCDCFG3_HFPW_OFFSET) + 1);
}

static u32 atmel_hlcdfb_get_rgbmode(struct fb_info *info)
{
	u32 value = 0;

	switch (info->bits_per_pixel) {
	case 1:
		value = LCDC_BASECFG1_CLUTMODE_1BPP | LCDC_BASECFG1_CLUTEN;
		break;
	case 2:
		value = LCDC_BASECFG1_CLUTMODE_2BPP | LCDC_BASECFG1_CLUTEN;
		break;
	case 4:
		value = LCDC_BASECFG1_CLUTMODE_4BPP | LCDC_BASECFG1_CLUTEN;
		break;
	case 8:
		value = LCDC_BASECFG1_CLUTMODE_8BPP | LCDC_BASECFG1_CLUTEN;
		break;
	case 12:
		value = LCDC_BASECFG1_RGBMODE_12BPP_RGB_444;
		break;
	case 16:
		if (info->transp.offset)
			value = LCDC_BASECFG1_RGBMODE_16BPP_ARGB_4444;
		else
			value = LCDC_BASECFG1_RGBMODE_16BPP_RGB_565;
		break;
	case 18:
		value = LCDC_BASECFG1_RGBMODE_18BPP_RGB_666_PACKED;
		break;
	case 24:
		value = LCDC_BASECFG1_RGBMODE_24BPP_RGB_888_PACKED;
		break;
	case 32:
		value = LCDC_BASECFG1_RGBMODE_32BPP_ARGB_8888;
		break;
	default:
		dev_err(&info->dev, "Cannot set video mode for %dbpp\n",
			info->bits_per_pixel);
		break;
	}

	return value;
}

static void atmel_hlcdfb_setup_core_base(struct fb_info *info)
{
	struct atmel_lcdfb_info *sinfo = info->priv;
	struct fb_videomode *mode = info->mode;
	unsigned long value;
	unsigned long clk_value_khz;

	dev_dbg(&info->dev, "%s:\n", __func__);
	/* Set pixel clock */
	clk_value_khz = clk_get_rate(sinfo->lcdc_clk) / 1000;

	value = DIV_ROUND_CLOSEST(clk_value_khz, PICOS2KHZ(mode->pixclock));

	if (value < 1) {
		dev_notice(&info->dev, "using system clock as pixel clock\n");
		value = LCDC_LCDCFG0_CLKPWMSEL;
	} else {
		mode->pixclock = KHZ2PICOS(clk_value_khz / value);
		dev_dbg(&info->dev, "  updated pixclk:     %lu KHz\n",
				PICOS2KHZ(mode->pixclock));
		value = value - 2;
		dev_dbg(&info->dev, "  * programming CLKDIV = 0x%08lx\n",
					value);
		value = (value << LCDC_LCDCFG0_CLKDIV_OFFSET);
	}
	value |= LCDC_LCDCFG0_CGDISBASE;
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG0, value);

	/* Initialize control register 5 */
	/* In 9x5, the lcdcon2 will use for LCDCFG5 */
	value = sinfo->lcdcon2;
	value |= (sinfo->guard_time << LCDC_LCDCFG5_GUARDTIME_OFFSET)
		| LCDC_LCDCFG5_DISPDLY
		| LCDC_LCDCFG5_VSPDLYS;

	if (!(mode->sync & FB_SYNC_HOR_HIGH_ACT))
		value |= LCDC_LCDCFG5_HSPOL;
	if (!(mode->sync & FB_SYNC_VERT_HIGH_ACT))
		value |= LCDC_LCDCFG5_VSPOL;

	dev_dbg(&info->dev, "  * LCDC_LCDCFG5 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG5, value);

	/* Vertical & Horizontal Timing */
	value = (mode->vsync_len - 1) << LCDC_LCDCFG1_VSPW_OFFSET;
	value |= (mode->hsync_len - 1) << LCDC_LCDCFG1_HSPW_OFFSET;
	dev_dbg(&info->dev, "  * LCDC_LCDCFG1 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG1, value);

	value = (mode->upper_margin) << LCDC_LCDCFG2_VBPW_OFFSET;
	value |= (mode->lower_margin - 1) << LCDC_LCDCFG2_VFPW_OFFSET;
	dev_dbg(&info->dev, "  * LCDC_LCDCFG2 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG2, value);

	value = (mode->left_margin - 1) << LCDC_LCDCFG3_HBPW_OFFSET;
	value |= (mode->right_margin - 1) << LCDC_LCDCFG3_HFPW_OFFSET;
	dev_dbg(&info->dev, "  * LCDC_LCDCFG3 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG3, value);

	/* Display size */
	value = (mode->yres - 1) << LCDC_LCDCFG4_RPF_OFFSET;
	value |= (mode->xres - 1) << LCDC_LCDCFG4_PPL_OFFSET;
	dev_dbg(&info->dev, "  * LCDC_LCDCFG4 = %08lx\n", value);
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG4, value);

	lcdc_writel(sinfo, ATMEL_LCDC_BASECFG0, LCDC_BASECFG0_BLEN_AHB_INCR16 | LCDC_BASECFG0_DLBO);
	lcdc_writel(sinfo, ATMEL_LCDC_BASECFG1, atmel_hlcdfb_get_rgbmode(info));
	lcdc_writel(sinfo, ATMEL_LCDC_BASECFG2, 0);
	lcdc_writel(sinfo, ATMEL_LCDC_BASECFG3, 0);	/* Default color */
	lcdc_writel(sinfo, ATMEL_LCDC_BASECFG4, LCDC_BASECFG4_DMA);

	/* Disable all interrupts */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDIDR, ~0UL);
	lcdc_writel(sinfo, ATMEL_LCDC_BASEIDR, ~0UL);
	/* Enable BASE LAYER overflow interrupts, if want to enable DMA interrupt, also need set it at LCDC_BASECTRL reg */
	lcdc_writel(sinfo, ATMEL_LCDC_BASEIER, LCDC_BASEIER_OVR);
	/* FIXME: Let video-driver register a callback */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDIER, LCDC_LCDIER_FIFOERRIE |
				LCDC_LCDIER_BASEIE | LCDC_LCDIER_HEOIE);
}


static void atmel_hlcdfb_init_contrast(struct atmel_lcdfb_info *sinfo)
{
	/* have some default contrast/backlight settings */
	lcdc_writel(sinfo, ATMEL_LCDC_LCDCFG6, LCDC_LCDCFG6_PWMPOL |
		(ATMEL_LCDC_CVAL_DEFAULT << LCDC_LCDCFG6_PWMCVAL_OFFSET));
}

struct atmel_lcdfb_devdata atmel_hlcdfb_data = {
	.start = atmel_hlcdfb_start,
	.stop = atmel_hlcdfb_stop,
	.update_dma = atmel_hlcdfb_update_dma,
	.setup_core = atmel_hlcdfb_setup_core_base,
	.init_contrast = atmel_hlcdfb_init_contrast,
	.limit_screeninfo = atmel_hlcdfb_limit_screeninfo,
	.dma_desc_size = sizeof(struct atmel_hlcd_dma_desc),
};

static int atmel_hlcdc_probe(struct device_d *dev)
{
	return atmel_lcdc_register(dev, &atmel_hlcdfb_data);
}

static struct driver_d atmel_hlcdc_driver = {
	.name	= "atmel_hlcdfb",
	.probe	= atmel_hlcdc_probe,
};
device_platform_driver(atmel_hlcdc_driver);
