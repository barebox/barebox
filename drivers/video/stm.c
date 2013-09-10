/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <jbe@pengutronix.de>
 *
 * This is based on code from:
 * Author: Vitaly Wool <vital@embeddedalley.com>
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 *
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
#include <common.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <xfuncs.h>
#include <io.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/imx-regs.h>
#include <mach/fb.h>

#define HW_LCDIF_CTRL 0x00
# define CTRL_SFTRST (1 << 31)
# define CTRL_CLKGATE (1 << 30)
# define CTRL_BYPASS_COUNT (1 << 19)
# define CTRL_VSYNC_MODE (1 << 18)
# define CTRL_DOTCLK_MODE (1 << 17)
# define CTRL_DATA_SELECT (1 << 16)
# define SET_BUS_WIDTH(x) (((x) & 0x3) << 10)
# define SET_WORD_LENGTH(x) (((x) & 0x3) << 8)
# define GET_WORD_LENGTH(x) (((x) >> 8) & 0x3)
# define CTRL_MASTER (1 << 5)
# define CTRL_DF16 (1 << 3)
# define CTRL_DF18 (1 << 2)
# define CTRL_DF24 (1 << 1)
# define CTRL_RUN (1 << 0)

#define HW_LCDIF_CTRL1 0x10
# define CTRL1_FIFO_CLEAR (1 << 21)
# define SET_BYTE_PACKAGING(x) (((x) & 0xf) << 16)
# define GET_BYTE_PACKAGING(x) (((x) >> 16) & 0xf)
# define CTRL1_RESET (1 << 0)

#ifdef CONFIG_ARCH_IMX28
# define HW_LCDIF_CTRL2 0x20
# define HW_LCDIF_TRANSFER_COUNT 0x30
#endif
#ifdef CONFIG_ARCH_IMX23
# define HW_LCDIF_TRANSFER_COUNT 0x20
#endif
# define SET_VCOUNT(x) (((x) & 0xffff) << 16)
# define SET_HCOUNT(x) ((x) & 0xffff)

#ifdef CONFIG_ARCH_IMX28
# define HW_LCDIF_CUR_BUF 0x40
# define HW_LCDIF_NEXT_BUF 0x50
#endif
#ifdef CONFIG_ARCH_IMX23
# define HW_LCDIF_CUR_BUF 0x30
# define HW_LCDIF_NEXT_BUF 0x40
#endif

#define HW_LCDIF_TIMING 0x60
# define SET_CMD_HOLD(x) (((x) & 0xff) << 24)
# define SET_CMD_SETUP(x) (((x) & 0xff) << 16)
# define SET_DATA_HOLD(x) (((x) & 0xff) << 8)
# define SET_DATA_SETUP(x) ((x) & 0xff)

#define HW_LCDIF_VDCTRL0 0x70
# define VDCTRL0_ENABLE_PRESENT (1 << 28)
# define VDCTRL0_VSYNC_POL (1 << 27) /* 0 = low active, 1 = high active */
# define VDCTRL0_HSYNC_POL (1 << 26) /* 0 = low active, 1 = high active */
# define VDCTRL0_DOTCLK_POL (1 << 25) /* 0 = output@falling, capturing@rising edge */
# define VDCTRL0_ENABLE_POL (1 << 24) /* 0 = low active, 1 = high active */
# define VDCTRL0_VSYNC_PERIOD_UNIT (1 << 21)
# define VDCTRL0_VSYNC_PULSE_WIDTH_UNIT (1 << 20)
# define VDCTRL0_HALF_LINE (1 << 19)
# define VDCTRL0_HALF_LINE_MODE (1 << 18)
# define SET_VSYNC_PULSE_WIDTH(x) ((x) & 0x3ffff)

#define HW_LCDIF_VDCTRL1 0x80

#define HW_LCDIF_VDCTRL2 0x90
#ifdef CONFIG_ARCH_IMX28
# define SET_HSYNC_PULSE_WIDTH(x) (((x) & 0x3fff) << 18)
#endif
#ifdef CONFIG_ARCH_IMX23
# define SET_HSYNC_PULSE_WIDTH(x) (((x) & 0xff) << 24)
#endif
# define SET_HSYNC_PERIOD(x) ((x) & 0x3ffff)

#define HW_LCDIF_VDCTRL3 0xa0
# define VDCTRL3_MUX_SYNC_SIGNALS (1 << 29)
# define VDCTRL3_VSYNC_ONLY (1 << 28)
# define SET_HOR_WAIT_CNT(x) (((x) & 0xfff) << 16)
# define SET_VERT_WAIT_CNT(x) ((x) & 0xffff)

#define HW_LCDIF_VDCTRL4 0xb0
#ifdef CONFIG_ARCH_IMX28
# define SET_DOTCLK_DLY(x) (((x) & 0x7) << 29)
#endif
# define VDCTRL4_SYNC_SIGNALS_ON (1 << 18)
# define SET_DOTCLK_H_VALID_DATA_CNT(x) ((x) & 0x3ffff)

#define HW_LCDIF_DVICTRL0 0xc0
#define HW_LCDIF_DVICTRL1 0xd0
#define HW_LCDIF_DVICTRL2 0xe0
#define HW_LCDIF_DVICTRL3 0xf0
#define HW_LCDIF_DVICTRL4 0x100

#ifdef CONFIG_ARCH_IMX28
# define HW_LCDIF_DATA 0x180
#endif
#ifdef CONFIG_ARCH_IMX23
# define HW_LCDIF_DATA 0x1b0
#endif

#ifdef CONFIG_ARCH_IMX28
# define HW_LCDIF_DEBUG0 0x1d0
#endif
#ifdef CONFIG_ARCH_IMX23
# define HW_LCDIF_DEBUG0 0x1f0
#endif
# define DEBUG_HSYNC (1 < 26)
# define DEBUG_VSYNC (1 < 25)

#define RED 0
#define GREEN 1
#define BLUE 2
#define TRANSP 3

struct imxfb_info {
	void __iomem *base;
	unsigned memory_size;
	struct fb_info info;
	struct device_d *hw_dev;
	struct imx_fb_platformdata *pdata;
	struct clk *clk;
};

/* the RGB565 true colour mode */
static const struct fb_bitfield def_rgb565[] = {
	[RED] = {
		.offset = 11,
		.length = 5,
	},
	[GREEN] = {
		.offset = 5,
		.length = 6,
	},
	[BLUE] = {
		.offset = 0,
		.length = 5,
	},
	[TRANSP] = {	/* no support for transparency */
		.length = 0,
	}
};

/* the RGB666 true colour mode */
static const struct fb_bitfield def_rgb666[] = {
	[RED] = {
		.offset = 16,
		.length = 6,
	},
	[GREEN] = {
		.offset = 8,
		.length = 6,
	},
	[BLUE] = {
		.offset = 0,
		.length = 6,
	},
	[TRANSP] = {	/* no support for transparency */
		.length = 0,
	}
};

/* the RGB888 true colour mode */
static const struct fb_bitfield def_rgb888[] = {
	[RED] = {
		.offset = 16,
		.length = 8,
	},
	[GREEN] = {
		.offset = 8,
		.length = 8,
	},
	[BLUE] = {
		.offset = 0,
		.length = 8,
	},
	[TRANSP] = {	/* no support for transparency */
		.length = 0,
	}
};

static inline unsigned calc_line_length(unsigned ppl, unsigned bpp)
{
	if (bpp == 24)
		bpp = 32;
	return (ppl * bpp) >> 3;
}

static void stmfb_enable_controller(struct fb_info *fb_info)
{
	struct imxfb_info *fbi = fb_info->priv;
	uint32_t reg, last_reg;
	unsigned loop, edges;

	/*
	 * Sometimes some data is still present in the FIFO. This leads into
	 * a correct but shifted picture. Clearing the FIFO helps
	 */
	writel(CTRL1_FIFO_CLEAR, fbi->base + HW_LCDIF_CTRL1 + STMP_OFFSET_REG_SET);

	/* if it was disabled, re-enable the mode again */
	reg = readl(fbi->base + HW_LCDIF_CTRL);
	reg |= CTRL_DOTCLK_MODE;
	writel(reg, fbi->base + HW_LCDIF_CTRL);

	/* enable the SYNC signals first, then the DMA engine */
	reg = readl(fbi->base + HW_LCDIF_VDCTRL4);
	reg |= VDCTRL4_SYNC_SIGNALS_ON;
	writel(reg, fbi->base + HW_LCDIF_VDCTRL4);

	/*
	 * Give the attached LC display or monitor a chance to sync into
	 * our signals.
	 * Wait for at least 2 VSYNCs = four VSYNC edges
	 */
	edges = 4;

	while (edges != 0) {
		loop = 800;
		last_reg = readl(fbi->base + HW_LCDIF_DEBUG0) & DEBUG_VSYNC;
		do {
			reg = readl(fbi->base + HW_LCDIF_DEBUG0) & DEBUG_VSYNC;
			if (reg != last_reg)
				break;
			last_reg = reg;
			loop--;
		} while (loop != 0);
		edges--;
	}

	/* stop FIFO reset */
	writel(CTRL1_FIFO_CLEAR, fbi->base + HW_LCDIF_CTRL1 + STMP_OFFSET_REG_CLR);

	/* enable LCD using LCD_RESET signal*/
	if (fbi->pdata->flags & USE_LCD_RESET)
		writel(CTRL1_RESET,  fbi->base + HW_LCDIF_CTRL1 + STMP_OFFSET_REG_SET);

	/* start the engine right now */
	writel(CTRL_RUN, fbi->base + HW_LCDIF_CTRL + STMP_OFFSET_REG_SET);

	if (fbi->pdata->enable)
		fbi->pdata->enable(1);
}

static void stmfb_disable_controller(struct fb_info *fb_info)
{
	struct imxfb_info *fbi = fb_info->priv;
	unsigned loop;
	uint32_t reg;


	/* disable LCD using LCD_RESET signal*/
	if (fbi->pdata->flags & USE_LCD_RESET)
		writel(CTRL1_RESET,  fbi->base + HW_LCDIF_CTRL1 + STMP_OFFSET_REG_CLR);

	if (fbi->pdata->enable)
		fbi->pdata->enable(0);

	/*
	 * Even if we disable the controller here, it will still continue
	 * until its FIFOs are running out of data
	 */
	reg = readl(fbi->base + HW_LCDIF_CTRL);
	reg &= ~CTRL_DOTCLK_MODE;
	writel(reg, fbi->base + HW_LCDIF_CTRL);

	loop = 1000;
	while (loop) {
		reg = readl(fbi->base + HW_LCDIF_CTRL);
		if (!(reg & CTRL_RUN))
			break;
		loop--;
	}

	reg = readl(fbi->base + HW_LCDIF_VDCTRL4);
	reg &= ~VDCTRL4_SYNC_SIGNALS_ON;
	writel(reg, fbi->base + HW_LCDIF_VDCTRL4);
}

static int stmfb_activate_var(struct fb_info *fb_info)
{
	struct imxfb_info *fbi = fb_info->priv;
	struct imx_fb_platformdata *pdata = fbi->pdata;
	struct fb_videomode *mode = fb_info->mode;
	uint32_t reg;
	unsigned size;

	/*
	 * we need at least this amount of memory for the framebuffer
	 */
	size = calc_line_length(mode->xres, fb_info->bits_per_pixel) *
		mode->yres;

	if (pdata->fixed_screen) {
		if (pdata->fixed_screen_size < size)
			return -ENOMEM;
		fb_info->screen_base = pdata->fixed_screen;
		fbi->memory_size = pdata->fixed_screen_size;
	} else {
		fb_info->screen_base = xrealloc(fb_info->screen_base, size);
		fbi->memory_size = size;
	}

	/** @todo ensure HCLK is active at this point of time! */

	size = clk_set_rate(fbi->clk, PICOS2KHZ(mode->pixclock) * 1000);
	if (size != 0) {
		dev_dbg(fbi->hw_dev, "Unable to set a valid pixel clock\n");
		return -EINVAL;
	}

	/*
	 * bring the controller out of reset and
	 * configure it into DOTCLOCK mode
	 */
	reg = CTRL_BYPASS_COUNT |	/* always in DOTCLOCK mode */
		CTRL_DOTCLK_MODE;
	writel(reg, fbi->base + HW_LCDIF_CTRL);

	/* master mode only */
	reg |= CTRL_MASTER;

	/*
	 * Configure videomode and interface mode
	 */
	reg |= SET_BUS_WIDTH(pdata->ld_intf_width);
	switch (fb_info->bits_per_pixel) {
	case 8:
		reg |= SET_WORD_LENGTH(1);
		/** @todo refer manual page 2046 for 8 bpp modes */
		dev_dbg(fbi->hw_dev, "8 bpp mode not supported yet\n");
		break;
	case 16:
		pr_debug("Setting up an RGB565 mode\n");
		reg |= SET_WORD_LENGTH(0);
		reg &= ~CTRL_DF16; /* we assume RGB565 */
		writel(SET_BYTE_PACKAGING(0xf), fbi->base + HW_LCDIF_CTRL1);
		fb_info->red = def_rgb565[RED];
		fb_info->green = def_rgb565[GREEN];
		fb_info->blue = def_rgb565[BLUE];
		fb_info->transp =  def_rgb565[TRANSP];
		break;
	case 24:
	case 32:
		pr_debug("Setting up an RGB888/666 mode\n");
		reg |= SET_WORD_LENGTH(3);
		switch (pdata->ld_intf_width) {
		case STMLCDIF_8BIT:
			dev_dbg(fbi->hw_dev,
				"Unsupported LCD bus width mapping\n");
			break;
		case STMLCDIF_16BIT:
		case STMLCDIF_18BIT:
			/* 24 bit to 18 bit mapping
			 * which means: ignore the upper 2 bits in
			 * each colour component
			 */
			reg |= CTRL_DF24;
			fb_info->red = def_rgb666[RED];
			fb_info->green = def_rgb666[GREEN];
			fb_info->blue = def_rgb666[BLUE];
			fb_info->transp =  def_rgb666[TRANSP];
			break;
		case STMLCDIF_24BIT:
			/* real 24 bit */
			fb_info->red = def_rgb888[RED];
			fb_info->green = def_rgb888[GREEN];
			fb_info->blue = def_rgb888[BLUE];
			fb_info->transp =  def_rgb888[TRANSP];
			break;
		}
		/* do not use packed pixels = one pixel per word instead */
		writel(SET_BYTE_PACKAGING(0x7), fbi->base + HW_LCDIF_CTRL1);
		break;
	default:
		dev_dbg(fbi->hw_dev, "Unhandled colour depth of %u\n",
			fb_info->bits_per_pixel);
		return -EINVAL;
	}
	writel(reg, fbi->base + HW_LCDIF_CTRL);
	pr_debug("Setting up CTRL to %08X\n", reg);

	writel(SET_VCOUNT(mode->yres) |
		SET_HCOUNT(mode->xres), fbi->base + HW_LCDIF_TRANSFER_COUNT);

	reg = VDCTRL0_ENABLE_PRESENT |	/* always in DOTCLOCK mode */
		VDCTRL0_VSYNC_PERIOD_UNIT |
		VDCTRL0_VSYNC_PULSE_WIDTH_UNIT;
	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		reg |= VDCTRL0_HSYNC_POL;
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		reg |= VDCTRL0_VSYNC_POL;
	if (mode->sync & FB_SYNC_DE_HIGH_ACT)
		reg |= VDCTRL0_ENABLE_POL;
	if (mode->sync & FB_SYNC_CLK_INVERT)
		reg |= VDCTRL0_DOTCLK_POL;

	reg |= SET_VSYNC_PULSE_WIDTH(mode->vsync_len);
	writel(reg, fbi->base + HW_LCDIF_VDCTRL0);
	pr_debug("Setting up VDCTRL0 to %08X\n", reg);

	/* frame length in lines */
	writel(mode->upper_margin + mode->vsync_len + mode->lower_margin +
			mode->yres,
		fbi->base + HW_LCDIF_VDCTRL1);

	/* line length in units of clocks or pixels */
	writel(SET_HSYNC_PULSE_WIDTH(mode->hsync_len) |
		SET_HSYNC_PERIOD(mode->left_margin + mode->hsync_len +
			mode->right_margin + mode->xres),
		fbi->base + HW_LCDIF_VDCTRL2);

	writel(SET_HOR_WAIT_CNT(mode->left_margin + mode->hsync_len) |
		SET_VERT_WAIT_CNT(mode->upper_margin + mode->vsync_len),
		fbi->base + HW_LCDIF_VDCTRL3);

	writel(
#ifdef CONFIG_ARCH_IMX28
		SET_DOTCLK_DLY(pdata->dotclk_delay) |
#endif
		SET_DOTCLK_H_VALID_DATA_CNT(mode->xres),
		fbi->base + HW_LCDIF_VDCTRL4);

	writel((uint32_t)fb_info->screen_base, fbi->base + HW_LCDIF_CUR_BUF);
	writel((uint32_t)fb_info->screen_base, fbi->base + HW_LCDIF_NEXT_BUF);

	return 0;
}

static void stmfb_info(struct device_d *hw_dev)
{
	struct imx_fb_platformdata *pdata = hw_dev->platform_data;
	unsigned u;

	printf(" Supported video modes:\n");
	for (u = 0; u < pdata->mode_cnt; u++)
		printf("  - '%s': %u x %u\n", pdata->mode_list[u].name,
			pdata->mode_list[u].xres, pdata->mode_list[u].yres);
}

/*
 * There is only one video hardware instance available.
 * It makes no sense to dynamically allocate this data
 */
static struct fb_ops imxfb_ops = {
	.fb_activate_var = stmfb_activate_var,
	.fb_enable = stmfb_enable_controller,
	.fb_disable = stmfb_disable_controller,
};

static struct imxfb_info fbi = {
	.info = {
		.fbops = &imxfb_ops,
	},
};

static int stmfb_probe(struct device_d *hw_dev)
{
	struct imx_fb_platformdata *pdata = hw_dev->platform_data;
	int ret;

	/* just init */
	fbi.info.priv = &fbi;

	/* add runtime hardware info */
	fbi.hw_dev = hw_dev;
	fbi.base = dev_request_mem_region(hw_dev, 0);
	fbi.pdata = pdata;
	fbi.clk = clk_get(hw_dev, NULL);
	if (IS_ERR(fbi.clk))
		return PTR_ERR(fbi.clk);
	clk_enable(fbi.clk);

	/* add runtime video info */
	fbi.info.mode_list = pdata->mode_list;
	fbi.info.num_modes = pdata->mode_cnt;
	fbi.info.mode = &fbi.info.mode_list[0];
	fbi.info.xres = fbi.info.mode->xres;
	fbi.info.yres = fbi.info.mode->yres;
	if (pdata->bits_per_pixel)
		fbi.info.bits_per_pixel = pdata->bits_per_pixel;
	else
		fbi.info.bits_per_pixel = 16;

	hw_dev->info = stmfb_info;

	ret = register_framebuffer(&fbi.info);
	if (ret != 0) {
		dev_err(hw_dev, "Failed to register framebuffer\n");
		return -EINVAL;
	}

	return 0;
}

static struct driver_d stmfb_driver = {
	.name	= "stmfb",
	.probe	= stmfb_probe,
};
device_platform_driver(stmfb_driver);

/**
 * @file
 * @brief LCDIF driver for i.MX23 and i.MX28
 *
 * The LCDIF support four modes of operation
 * - MPU interface (to drive smart displays) -> not supported yet
 * - VSYNC interface (like MPU interface plus Vsync) -> not supported yet
 * - Dotclock interface (to drive LC displays with RGB data and sync signals)
 * - DVI (to drive ITU-R BT656)  -> not supported yet
 *
 * This driver depends on a correct setup of the pins used for this purpose
 * (platform specific).
 *
 * For the developer: Don't forget to set the data bus width to the display
 * in the imx_fb_platformdata structure. You will else end up with ugly colours.
 * If you fight against jitter you can vary the clock delay. This is a feature
 * of the i.MX28 and you can vary it between 2 ns ... 8 ns in 2 ns steps. Give
 * the required value in the imx_fb_platformdata structure.
 */
