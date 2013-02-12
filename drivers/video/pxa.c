/*
 * Copyright (C) 2010 Marc Kleine-Budde, Pengutronix
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
 *
 * Derived from the linux-2.6 pxa framebuffer driver:
 *
 * Copyright (C) 1999 Eric A. Thomas.
 * Copyright (C) 2004 Jean-Frederic Clere.
 * Copyright (C) 2004 Ian Campbell.
 * Copyright (C) 2004 Jeff Lackey.
 * Based on sa1100fb.c Copyright (C) 1999 Eric A. Thomas
 *   which in turn is:
 * Based on acornfb.c Copyright (C) Russell King.
 *
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <fb.h>
#include <init.h>
#include <malloc.h>

#include <mach/clock.h>
#include <mach/pxa-regs.h>
#include <mach/regs-lcd.h>
#include <mach/pxafb.h>

#include <asm/io.h>
#include <asm/mmu.h>
#include <asm-generic/div64.h>

/* PXA LCD DMA descriptor */
struct pxafb_dma_descriptor {
	unsigned int fdadr;
	unsigned int fsadr;
	unsigned int fidr;
	unsigned int ldcmd;
};

enum {
	PAL_NONE	= -1,
	PAL_BASE	= 0,
	PAL_MAX,
};

enum {
	DMA_BASE	= 0,
	DMA_UPPER	= 0,
	DMA_LOWER	= 1,
	DMA_MAX,
};

struct pxafb_dma_buff {
	struct pxafb_dma_descriptor dma_desc[DMA_MAX * 2];
};

struct pxafb_info {
	void __iomem		*regs;

	struct pxafb_dma_buff	*dma_buff;
	dma_addr_t		fdadr[DMA_MAX * 2];

	u32			lccr0;
	u32			lccr3;
	u32			lccr4;

	u32			reg_lccr0;
	u32			reg_lccr1;
	u32			reg_lccr2;
	u32			reg_lccr3;
	u32			reg_lccr4;

	struct pxafb_videomode	*mode;
	struct fb_info		info;
	struct device_d		*dev;

	void			(*lcd_power)(int);
	void			(*backlight_power)(int);
};

#define info_to_pxafb_info(_info)	container_of(_info, struct pxafb_info, info)

static inline unsigned long
lcd_readl(struct pxafb_info *fbi, unsigned int off)
{
	return __raw_readl(fbi->regs + off);
}

static inline void
lcd_writel(struct pxafb_info *fbi, unsigned int off, unsigned long val)
{
	__raw_writel(val, fbi->regs + off);
}

static inline void pxafb_backlight_power(struct pxafb_info *fbi, int on)
{
	pr_debug("pxafb: backlight o%s\n", on ? "n" : "ff");

	if (fbi->backlight_power)
		fbi->backlight_power(on);
}

static inline void pxafb_lcd_power(struct pxafb_info *fbi, int on)
{
	pr_debug("pxafb: LCD power o%s\n", on ? "n" : "ff");

	if (fbi->lcd_power)
		fbi->lcd_power(on);
}

static void pxafb_enable_controller(struct fb_info *info)
{
	struct pxafb_info *fbi = info_to_pxafb_info(info);

	pr_debug("pxafb: Enabling LCD controller\n");
	pr_debug("fdadr0 0x%08x\n", (unsigned int) fbi->fdadr[0]);
	pr_debug("fdadr1 0x%08x\n", (unsigned int) fbi->fdadr[1]);
	pr_debug("reg_lccr0 0x%08x\n", (unsigned int) fbi->reg_lccr0);
	pr_debug("reg_lccr1 0x%08x\n", (unsigned int) fbi->reg_lccr1);
	pr_debug("reg_lccr2 0x%08x\n", (unsigned int) fbi->reg_lccr2);
	pr_debug("reg_lccr3 0x%08x\n", (unsigned int) fbi->reg_lccr3);
	pr_debug("dma_desc  0x%08x\n", (unsigned int) &fbi->dma_buff->dma_desc[DMA_BASE]);

	/* enable LCD controller clock */
	CKEN |= CKEN_LCD;

	if (fbi->lccr0 & LCCR0_LCDT)
		return;

	/* Sequence from 11.7.10 */
	lcd_writel(fbi, LCCR4, fbi->reg_lccr4);
	lcd_writel(fbi, LCCR3, fbi->reg_lccr3);
	lcd_writel(fbi, LCCR2, fbi->reg_lccr2);
	lcd_writel(fbi, LCCR1, fbi->reg_lccr1);
	lcd_writel(fbi, LCCR0, fbi->reg_lccr0 & ~LCCR0_ENB);

	lcd_writel(fbi, FDADR0, fbi->fdadr[0]);
	lcd_writel(fbi, FDADR1, fbi->fdadr[1]);
	lcd_writel(fbi, LCCR0, fbi->reg_lccr0 | LCCR0_ENB);

	pxafb_lcd_power(fbi, 1);
	pxafb_backlight_power(fbi, 1);
}

static void pxafb_disable_controller(struct fb_info *info)
{
	struct pxafb_info *fbi = info_to_pxafb_info(info);
	u32 lccr0;

	pxafb_backlight_power(fbi, 0);
	pxafb_lcd_power(fbi, 0);

	/* Clear LCD Status Register */
	lcd_writel(fbi, LCSR, 0xffffffff);

	lccr0 = lcd_readl(fbi, LCCR0) & ~LCCR0_LDM;
	lcd_writel(fbi, LCCR0, lccr0);
	lcd_writel(fbi, LCCR0, lccr0 | LCCR0_DIS);

	/* disable LCD controller clock */
	CKEN &= ~CKEN_LCD;
}

static int setup_frame_dma(struct pxafb_info *fbi, int dma, int pal,
			   unsigned long start, size_t size)
{
	struct pxafb_dma_descriptor *dma_desc;

	if (dma < 0 || dma >= DMA_MAX * 2)
		return -EINVAL;

	dma_desc = &fbi->dma_buff->dma_desc[dma];

	dma_desc->fsadr = start;
	dma_desc->fidr  = 0;
	dma_desc->ldcmd = size;

	if (pal < 0 || pal >= PAL_MAX * 2) {
		dma_desc->fdadr = virt_to_phys(dma_desc);
		fbi->fdadr[dma] = virt_to_phys(dma_desc);
	} else {
#if 0
		struct pxafb_dma_descriptor *pal_desc;

		pal_desc = &fbi->dma_buff->pal_desc[pal];

		pal_desc->fsadr = fbi->dma_buff_phys + pal * PALETTE_SIZE;
		pal_desc->fidr = 0;

		if ((fbi->lccr4 & LCCR4_PAL_FOR_MASK) == LCCR4_PAL_FOR_0)
			pal_desc->ldcmd = fbi->palette_size * sizeof(u16);
		else
			pal_desc->ldcmd = fbi->palette_size * sizeof(u32);

		pal_desc->ldcmd |= LDCMD_PAL;

		/* flip back and forth between palette and frame buffer */
		pal_desc->fdadr = fbi->dma_buff_phys + dma_desc_off;
		dma_desc->fdadr = fbi->dma_buff_phys + pal_desc_off;
		fbi->fdadr[dma] = fbi->dma_buff_phys + dma_desc_off;
#endif
	}

	return 0;
}

static void setup_base_frame(struct pxafb_info *fbi)
{
	struct pxafb_videomode *mode = fbi->mode;
	struct fb_info *info = &fbi->info;
	int nbytes, dma, pal, bpp = info->bits_per_pixel;
	unsigned long line_length, offset;

	dma = DMA_BASE;
	pal = (bpp >= 16) ? PAL_NONE : PAL_BASE;

	line_length = info->xres * info->bits_per_pixel / 8;

	nbytes = line_length * mode->mode.yres;
	offset = virt_to_phys(info->screen_base);

	if (fbi->lccr0 & LCCR0_SDS) {
		nbytes = nbytes / 2;
		setup_frame_dma(fbi, dma + 1, PAL_NONE, offset + nbytes, nbytes);
	}

	setup_frame_dma(fbi, dma, pal, offset, nbytes);
}

/*
 * Calculate the PCD value from the clock rate (in picoseconds).
 * We take account of the PPCR clock setting.
 * From PXA Developer's Manual:
 *
 *   PixelClock =      LCLK
 *                -------------
 *                2 ( PCD + 1 )
 *
 *   PCD =      LCLK
 *         ------------- - 1
 *         2(PixelClock)
 *
 * Where:
 *   LCLK = LCD/Memory Clock
 *   PCD = LCCR3[7:0]
 *
 * PixelClock here is in Hz while the pixclock argument given is the
 * period in picoseconds. Hence PixelClock = 1 / ( pixclock * 10^-12 )
 *
 * The function get_lclk_frequency_10khz returns LCLK in units of
 * 10khz. Calling the result of this function lclk gives us the
 * following
 *
 *    PCD = (lclk * 10^4 ) * ( pixclock * 10^-12 )
 *          -------------------------------------- - 1
 *                          2
 *
 * Factoring the 10^4 and 10^-12 out gives 10^-8 == 1 / 100000000 as used below.
 */
static inline unsigned int get_pcd(struct pxafb_info *fbi,
				   unsigned int pixclock)
{
	unsigned long long pcd;

	/*
	 * FIXME: Need to take into account Double Pixel Clock mode
	 * (DPC) bit? or perhaps set it based on the various clock
	 * speeds
	 */
	pcd = (unsigned long long)(pxa_get_lcdclk() / 10000);
	pcd *= pixclock;
	do_div(pcd, 100000000 * 2);
	/* no need for this, since we should subtract 1 anyway. they cancel */
	/* pcd += 1; */ /* make up for integer math truncations */
	return (unsigned int)pcd;
}

static void setup_parallel_timing(struct pxafb_info *fbi)
{
	struct fb_info *info = &fbi->info;
	struct fb_videomode *mode = info->mode;

	unsigned int lines_per_panel, pcd = get_pcd(fbi, mode->pixclock);

	fbi->reg_lccr1 =
		LCCR1_DisWdth(mode->xres) +
		LCCR1_HorSnchWdth(mode->hsync_len) +
		LCCR1_BegLnDel(mode->left_margin) +
		LCCR1_EndLnDel(mode->right_margin);

	/*
	 * If we have a dual scan LCD, we need to halve
	 * the YRES parameter.
	 */
	lines_per_panel = mode->yres;
	if ((fbi->lccr0 & LCCR0_SDS) == LCCR0_Dual)
		lines_per_panel /= 2;

	fbi->reg_lccr2 =
		LCCR2_DisHght(lines_per_panel) +
		LCCR2_VrtSnchWdth(mode->vsync_len) +
		LCCR2_BegFrmDel(mode->upper_margin) +
		LCCR2_EndFrmDel(mode->lower_margin);

	fbi->reg_lccr3 = fbi->lccr3 |
		(mode->sync & FB_SYNC_HOR_HIGH_ACT ?
		 LCCR3_HorSnchH : LCCR3_HorSnchL) |
		(mode->sync & FB_SYNC_VERT_HIGH_ACT ?
		 LCCR3_VrtSnchH : LCCR3_VrtSnchL);

	if (pcd)
		fbi->reg_lccr3 |= LCCR3_PixClkDiv(pcd);
}

/* calculate pixel depth, transparency bit included, >=16bpp formats _only_ */
static inline int var_to_depth(struct fb_info *info)
{
	return info->red.length + info->green.length +
		info->blue.length + info->transp.length;
}

/* calculate 4-bit BPP value for LCCR3 and OVLxC1 */
static int pxafb_var_to_bpp(struct fb_info *info)
{
	int bpp = -EINVAL;

	switch (info->bits_per_pixel) {
	case 1:  bpp = 0; break;
	case 2:  bpp = 1; break;
	case 4:  bpp = 2; break;
	case 8:  bpp = 3; break;
	case 16: bpp = 4; break;
	case 24:
		switch (var_to_depth(info)) {
		case 18: bpp = 6; break; /* 18-bits/pixel packed */
		case 19: bpp = 8; break; /* 19-bits/pixel packed */
		case 24: bpp = 9; break;
		}
		break;
	case 32:
		switch (var_to_depth(info)) {
		case 18: bpp = 5; break; /* 18-bits/pixel unpacked */
		case 19: bpp = 7; break; /* 19-bits/pixel unpacked */
		case 25: bpp = 10; break;
		}
		break;
	}
	return bpp;
}

/*
 *  pxafb_var_to_lccr3():
 *    Convert a bits per pixel value to the correct bit pattern for LCCR3
 *
 *  NOTE: for PXA27x with overlays support, the LCCR3_PDFOR_x bits have an
 *  implication of the acutal use of transparency bit,  which we handle it
 *  here separatedly. See PXA27x Developer's Manual, Section <<7.4.6 Pixel
 *  Formats>> for the valid combination of PDFOR, PAL_FOR for various BPP.
 *
 *  Transparency for palette pixel formats is not supported at the moment.
 */
static uint32_t pxafb_var_to_lccr3(struct fb_info *info)
{
	int bpp = pxafb_var_to_bpp(info);
	uint32_t lccr3;

	if (bpp < 0)
		return 0;

	lccr3 = LCCR3_BPP(bpp);

	switch (var_to_depth(info)) {
	case 16: lccr3 |= info->transp.length ? LCCR3_PDFOR_3 : 0; break;
	case 18: lccr3 |= LCCR3_PDFOR_3; break;
	case 24: lccr3 |= info->transp.length ? LCCR3_PDFOR_2 : LCCR3_PDFOR_3;
		 break;
	case 19:
	case 25: lccr3 |= LCCR3_PDFOR_0; break;
	}
	return lccr3;
}

/*
 * Configures LCD Controller based on entries in fbi parameter.
 */
static int pxafb_activate_var(struct pxafb_info *fbi)
{
	struct fb_info *info = &fbi->info;

	setup_parallel_timing(fbi);

	setup_base_frame(fbi);

	fbi->reg_lccr0 = fbi->lccr0 |
		(LCCR0_LDM | LCCR0_SFM | LCCR0_IUM | LCCR0_EFM |
		 LCCR0_QDM | LCCR0_BM  | LCCR0_OUM);

	fbi->reg_lccr3 |= pxafb_var_to_lccr3(info);

	fbi->reg_lccr4 = lcd_readl(fbi, LCCR4) & ~LCCR4_PAL_FOR_MASK;
	fbi->reg_lccr4 |= (fbi->lccr4 & LCCR4_PAL_FOR_MASK);

	return 0;
}

#define SET_PIXFMT(v, r, g, b)					\
({								\
	(v)->blue.length   = (b); (v)->blue.offset = 0;		\
	(v)->green.length  = (g); (v)->green.offset = (b);	\
	(v)->red.length    = (r); (v)->red.offset = (b) + (g);	\
})

/*
 * set the RGBT bitfields of fb_var_screeninf according to
 * var->bits_per_pixel and given depth
 */
static void pxafb_set_pixfmt(struct fb_info *info)
{
	if (info->bits_per_pixel < 16) {
		/* indexed pixel formats */
		info->red.offset    = 0; info->red.length    = 8;
		info->green.offset  = 0; info->green.length  = 8;
		info->blue.offset   = 0; info->blue.length   = 8;
		info->transp.offset = 0; info->transp.length = 8;
	}

	switch (info->bits_per_pixel) {
	case 16: SET_PIXFMT(info, 5, 6, 5); break;	/* RGB565 */
	case 18: SET_PIXFMT(info, 6, 6, 6); break;	/* RGB666 */
	case 24: SET_PIXFMT(info, 8, 8, 8); break;	/* RGB888 */
	}
}

static void pxafb_decode_mach_info(struct pxafb_info *fbi,
				   struct pxafb_platform_data *inf)
{
	unsigned int lcd_conn = inf->lcd_conn;

	switch (lcd_conn & LCD_TYPE_MASK) {
	case LCD_TYPE_MONO_STN:
		fbi->lccr0 = LCCR0_CMS;
		break;
	case LCD_TYPE_MONO_DSTN:
		fbi->lccr0 = LCCR0_CMS | LCCR0_SDS;
		break;
	case LCD_TYPE_COLOR_STN:
		fbi->lccr0 = 0;
		break;
	case LCD_TYPE_COLOR_DSTN:
		fbi->lccr0 = LCCR0_SDS;
		break;
	case LCD_TYPE_COLOR_TFT:
		fbi->lccr0 = LCCR0_PAS;
		break;
	case LCD_TYPE_SMART_PANEL:
		fbi->lccr0 = LCCR0_LCDT | LCCR0_PAS;
		break;
	}

	if (lcd_conn == LCD_MONO_STN_8BPP)
		fbi->lccr0 |= LCCR0_DPD;

	fbi->lccr0 |= (lcd_conn & LCD_ALTERNATE_MAPPING) ? LCCR0_LDDALT : 0;

	fbi->lccr3 = LCCR3_Acb((inf->lcd_conn >> 10) & 0xff) |
		(lcd_conn & LCD_BIAS_ACTIVE_LOW) ? LCCR3_OEP : 0 |
		(lcd_conn & LCD_PCLK_EDGE_FALL)  ? LCCR3_PCP : 0;

	pxafb_set_pixfmt(&fbi->info);
}

static struct fb_ops pxafb_ops = {
	.fb_enable	= pxafb_enable_controller,
	.fb_disable	= pxafb_disable_controller,
};

static int pxafb_probe(struct device_d *dev)
{
	struct pxafb_platform_data *pdata = dev->platform_data;
	struct pxafb_info *fbi;
	struct fb_info *info;
	int ret;

	if (!pdata)
		return -ENODEV;

	fbi = xzalloc(sizeof(*fbi));
	info = &fbi->info;

	fbi->mode = pdata->mode;
	fbi->regs = dev_request_mem_region(dev, 0);

	fbi->dev = dev;
	fbi->lcd_power = pdata->lcd_power;
	fbi->backlight_power = pdata->backlight_power;
	info->mode = &pdata->mode->mode;
	info->fbops = &pxafb_ops;

	info->xres = pdata->mode->mode.xres;
	info->yres = pdata->mode->mode.yres;
	info->bits_per_pixel = pdata->mode->bpp;

	pxafb_decode_mach_info(fbi, pdata);

	dev_info(dev, "PXA Framebuffer driver\n");

	if (pdata->framebuffer)
		fbi->info.screen_base = pdata->framebuffer;
	else
		fbi->info.screen_base =
			PTR_ALIGN(dma_alloc_coherent(info->xres * info->yres *
						     (info->bits_per_pixel >> 3) + PAGE_SIZE),
				  PAGE_SIZE);

	fbi->dma_buff = PTR_ALIGN(dma_alloc_coherent(sizeof(struct pxafb_dma_buff) + 16),
				  16);

	pxafb_activate_var(fbi);

	ret = register_framebuffer(&fbi->info);
	if (ret < 0) {
		dev_err(dev, "failed to register framebuffer\n");
		return ret;
	}

	return 0;
}

static void pxafb_remove(struct device_d *dev)
{
}

static struct driver_d pxafb_driver = {
	.name	= "pxafb",
	.probe	= pxafb_probe,
	.remove	= pxafb_remove,
};
device_platform_driver(pxafb_driver);
