/*
 *  Freescale i.MX Frame Buffer device driver
 *
 *  Copyright (C) 2004 Sascha Hauer, Pengutronix
 *   Based on acornfb.c Copyright (C) Russell King.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Please direct your questions and comments on this driver to the following
 * email address:
 *
 *	linux-arm-kernel@lists.arm.linux.org.uk
 */

#include <common.h>
#include <fb.h>
#include <io.h>
#include <mach/imxfb.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/sizes.h>
#include <asm-generic/div64.h>

#define LCDC_SSA	0x00

#define LCDC_SIZE	0x04
#define SIZE_XMAX(x)	((((x) >> 4) & 0x3f) << 20)

#ifdef CONFIG_ARCH_IMX1
#define SIZE_YMAX(y)	((y) & 0x1ff)
#else
#define SIZE_YMAX(y)	((y) & 0x3ff)
#endif

#define LCDC_VPW	0x08
#define VPW_VPW(x)	((x) & 0x3ff)

#define LCDC_CPOS	0x0C
#define CPOS_CC1	(1<<31)
#define CPOS_CC0	(1<<30)
#define CPOS_OP		(1<<28)
#define CPOS_CXP(x)	(((x) & 3ff) << 16)

#ifdef CONFIG_ARCH_IMX1
#define CPOS_CYP(y)	((y) & 0x1ff)
#else
#define CPOS_CYP(y)	((y) & 0x3ff)
#endif

#define LCDC_LCWHB	0x10
#define LCWHB_BK_EN	(1<<31)
#define LCWHB_CW(w)	(((w) & 0x1f) << 24)
#define LCWHB_CH(h)	(((h) & 0x1f) << 16)
#define LCWHB_BD(x)	((x) & 0xff)

#define LCDC_LCHCC	0x14

#ifdef CONFIG_ARCH_IMX1
#define LCHCC_CUR_COL_R(r) (((r) & 0x1f) << 11)
#define LCHCC_CUR_COL_G(g) (((g) & 0x3f) << 5)
#define LCHCC_CUR_COL_B(b) ((b) & 0x1f)
#else
#define LCHCC_CUR_COL_R(r) (((r) & 0x3f) << 12)
#define LCHCC_CUR_COL_G(g) (((g) & 0x3f) << 6)
#define LCHCC_CUR_COL_B(b) ((b) & 0x3f)
#endif

#define LCDC_PCR	0x18

#define LCDC_HCR	0x1C
#define HCR_H_WIDTH(x)	(((x) & 0x3f) << 26)
#define HCR_H_WAIT_1(x)	(((x) & 0xff) << 8)
#define HCR_H_WAIT_2(x)	((x) & 0xff)

#define LCDC_VCR	0x20
#define VCR_V_WIDTH(x)	(((x) & 0x3f) << 26)
#define VCR_V_WAIT_1(x)	(((x) & 0xff) << 8)
#define VCR_V_WAIT_2(x)	((x) & 0xff)

#define LCDC_POS	0x24
#define POS_POS(x)	((x) & 1f)

#define LCDC_LSCR1	0x28
/* bit fields in imxfb.h */

#define LCDC_PWMR	0x2C
/* bit fields in imxfb.h */

#define LCDC_DMACR	0x30
/* bit fields in imxfb.h */

#define LCDC_RMCR	0x34

#ifdef CONFIG_ARCH_IMX1
#define RMCR_LCDC_EN	(1<<1)
#else
#define RMCR_LCDC_EN	0
#endif

#define RMCR_SELF_REF	(1<<0)

#define LCDC_LCDICR	0x38
#define LCDICR_INT_SYN	(1<<2)
#define LCDICR_INT_CON	(1)

#define LCDC_LCDISR	0x40
#define LCDISR_UDR_ERR	(1<<3)
#define LCDISR_ERR_RES	(1<<2)
#define LCDISR_EOF	(1<<1)
#define LCDISR_BOF	(1<<0)

#define LCDC_LGWSAR	0x50
#define LCDC_LGWSR	0x54
#define LCDC_LGWVPWR	0x58
#define LCDC_LGWPOR	0x5c
#define LCDC_LGWPR	0x60
#define LCDC_LGWCR	0x64
#define LGWCR_GWAV(alpha)	(((alpha) & 0xff) << 24)
#define LGWCR_GWE	(1 << 22)

#define LCDC_LGWDCR	0x68

/*
 * These are the bitfields for each
 * display depth that we support.
 */
struct imxfb_rgb {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

struct imxfb_info {
	void __iomem		*regs;

	struct clk		*ahb_clk;
	struct clk		*ipg_clk;
	struct clk		*per_clk;

	u_int			pcr;
	u_int			pwmr;
	u_int			lscr1;
	u_int			dmacr;
	u_int			cmap_inverse:1,
				cmap_static:1,
				unused:30;

	struct fb_info		info;
	struct device_d		*dev;

	void			(*enable)(int enable);

	unsigned int		alpha;

	struct fb_info		overlay;
};

#define IMX_NAME	"IMX"

/*
 * Minimum X and Y resolutions
 */
#define MIN_XRES	64
#define MIN_YRES	64

/* Actually this really is 18bit support, the lowest 2 bits of each colour
 * are unused in hardware. We claim to have 24bit support to make software
 * like X work, which does not support 18bit.
 */
static struct imxfb_rgb def_rgb_18 = {
	.red	= {.offset = 16, .length = 8,},
	.green	= {.offset = 8, .length = 8,},
	.blue	= {.offset = 0, .length = 8,},
	.transp = {.offset = 0, .length = 0,},
};

static struct imxfb_rgb def_rgb_16_tft = {
	.red	= {.offset = 11, .length = 5,},
	.green	= {.offset = 5, .length = 6,},
	.blue	= {.offset = 0, .length = 5,},
	.transp = {.offset = 0, .length = 0,},
};

static struct imxfb_rgb def_rgb_16_stn = {
	.red	= {.offset = 8, .length = 4,},
	.green	= {.offset = 4, .length = 4,},
	.blue	= {.offset = 0, .length = 4,},
	.transp = {.offset = 0, .length = 0,},
};

static struct imxfb_rgb def_rgb_8 = {
	.red	= {.offset = 0, .length = 8,},
	.green	= {.offset = 0, .length = 8,},
	.blue	= {.offset = 0, .length = 8,},
	.transp = {.offset = 0, .length = 0,},
};

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}


static int imxfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		   u_int trans, struct fb_info *info)
{
	struct imxfb_info *fbi = info->priv;
	int ret = 1;
	u32 val;

	/*
	 * If inverse mode was selected, invert all the colours
	 * rather than the register number.  The register number
	 * is what you poke into the framebuffer to produce the
	 * colour you requested.
	 */
	if (fbi->cmap_inverse) {
		red   = 0xffff - red;
		green = 0xffff - green;
		blue  = 0xffff - blue;
	}

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (info->grayscale)
		red = green = blue = (19595 * red + 38470 * green +
					7471 * blue) >> 16;

#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
	if (regno < 256) {
		val = (CNVT_TOHW(red, 4) << 8) |
		      (CNVT_TOHW(green,4) << 4) |
		      CNVT_TOHW(blue,  4);

		writel(val, fbi->regs + 0x800 + (regno << 2));
		ret = 0;
	}

	return ret;
}

static void imxfb_enable_controller(struct fb_info *info)
{
	struct imxfb_info *fbi = info->priv;

	writel(RMCR_LCDC_EN, fbi->regs + LCDC_RMCR);

	clk_enable(fbi->ahb_clk);
	clk_enable(fbi->ipg_clk);
	clk_enable(fbi->per_clk);

	if (fbi->enable)
		fbi->enable(1);
}

static void imxfb_disable_controller(struct fb_info *info)
{
	struct imxfb_info *fbi = info->priv;

	if (fbi->enable)
		fbi->enable(0);

	writel(0, fbi->regs + LCDC_RMCR);

	clk_disable(fbi->per_clk);
	clk_disable(fbi->ipg_clk);
	clk_disable(fbi->ahb_clk);
}

/*
 * imxfb_activate_var():
 *	Configures LCD Controller based on entries in var parameter.  Settings are
 *	only written to the controller if changes were made.
 */
static int imxfb_activate_var(struct fb_info *info)
{
	struct fb_videomode *mode = info->mode;
	struct imxfb_rgb *rgb;
	unsigned long lcd_clk;
	unsigned long long tmp;
	struct imxfb_info *fbi = info->priv;
	u32 pcr;

	/* physical screen start address	    */
	writel(VPW_VPW(mode->xres * info->bits_per_pixel / 8 / 4),
		fbi->regs + LCDC_VPW);

	writel(HCR_H_WIDTH(mode->hsync_len - 1) |
		HCR_H_WAIT_1(mode->right_margin - 1) |
		HCR_H_WAIT_2(mode->left_margin - 3),
		fbi->regs + LCDC_HCR);

	writel(VCR_V_WIDTH(mode->vsync_len) |
		VCR_V_WAIT_1(mode->lower_margin) |
		VCR_V_WAIT_2(mode->upper_margin),
		fbi->regs + LCDC_VCR);

	writel(SIZE_XMAX(info->xres) | SIZE_YMAX(info->yres),
			fbi->regs + LCDC_SIZE);

	writel(fbi->pwmr, fbi->regs + LCDC_PWMR);
	writel(fbi->lscr1, fbi->regs + LCDC_LSCR1);
	writel(fbi->dmacr, fbi->regs + LCDC_DMACR);
	writel((unsigned long)fbi->info.screen_base, fbi->regs + LCDC_SSA);

	/* panning offset 0 (0 pixel offset)        */
	writel(0x0, fbi->regs + LCDC_POS);

	/* disable hardware cursor */
	writel(readl(fbi->regs + LCDC_CPOS) & ~(CPOS_CC0 | CPOS_CC1),
		fbi->regs + LCDC_CPOS);

	lcd_clk = clk_get_rate(fbi->per_clk);

	tmp = mode->pixclock * (unsigned long long)lcd_clk;

	do_div(tmp, 1000000);

	if (do_div(tmp, 1000000) > 500000)
		tmp++;

	pcr = (unsigned int)tmp;
	if (--pcr > 0x3f) {
		pcr = 0x3f;
		printk(KERN_WARNING "Must limit pixel clock to %luHz\n",
				lcd_clk / pcr);
	}

	switch (info->bits_per_pixel) {
	case 32:
		pcr |= PCR_BPIX_18;
		rgb = &def_rgb_18;
		break;
	case 16:
	default:
#ifdef CONFIG_ARCH_IMX1
		pcr |= PCR_BPIX_12;
#else
		pcr |= PCR_BPIX_16;
#endif
		if (fbi->pcr & PCR_TFT)
			rgb = &def_rgb_16_tft;
		else
			rgb = &def_rgb_16_stn;
		break;
	case 8:
		pcr |= PCR_BPIX_8;
		rgb = &def_rgb_8;
		break;
	}

	writel(fbi->pcr | pcr, fbi->regs + LCDC_PCR);

	/*
	 * Copy the RGB parameters for this display
	 * from the machine specific parameters.
	 */
	info->red    = rgb->red;
	info->green  = rgb->green;
	info->blue   = rgb->blue;
	info->transp = rgb->transp;

	return 0;
}

static struct fb_ops imxfb_ops = {
	.fb_setcolreg	= imxfb_setcolreg,
	.fb_enable	= imxfb_enable_controller,
	.fb_disable	= imxfb_disable_controller,
	.fb_activate_var = imxfb_activate_var,
};

static int imxfb_allocate_fbbuffer(const struct device_d *dev,
				   struct fb_info *info, void *forcefb)
{
	size_t fbsize = info->xres * info->yres * (info->bits_per_pixel >> 3);

	/*
	 * The buffer must be completely contained in an aligned 4 MiB memory
	 * area.
	 */
	if (forcefb) {
		if (ALIGN((unsigned long)forcefb, SZ_4M) !=
		    ALIGN((unsigned long)forcefb + fbsize, SZ_4M)) {
			dev_err(dev,
				"provided frame buffer crosses a 4 MiB boundary\n");
			return -EINVAL;
		}
		info->screen_base = forcefb;
	} else {
		/*
		 * memalign implements a stricter condition on the memory
		 * allocation as necessary, but in the absense of a better
		 * function just use it.
		 */
		info->screen_base = memalign(fbsize, SZ_4M);
		if (!info->screen_base)
			return -ENOMEM;
		memset(info->screen_base, 0, fbsize);
	}
	return 0;
}

#ifdef CONFIG_IMXFB_DRIVER_VIDEO_IMX_OVERLAY
static void imxfb_overlay_enable_controller(struct fb_info *overlay)
{
	struct imxfb_info *fbi = overlay->priv;
	unsigned int tmp;

	tmp = readl(fbi->regs + LCDC_LGWCR);
	tmp |= LGWCR_GWE;
	writel(tmp , fbi->regs + LCDC_LGWCR);
}

static void imxfb_overlay_disable_controller(struct fb_info *overlay)
{
	struct imxfb_info *fbi = overlay->priv;
	unsigned int tmp;

	tmp = readl(fbi->regs + LCDC_LGWCR);
	tmp &= ~LGWCR_GWE;
	writel(tmp , fbi->regs + LCDC_LGWCR);
}

static int imxfb_overlay_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		   u_int trans, struct fb_info *info)
{
	return 0;
}

static struct fb_ops imxfb_overlay_ops = {
	.fb_setcolreg	= imxfb_overlay_setcolreg,
	.fb_enable	= imxfb_overlay_enable_controller,
	.fb_disable	= imxfb_overlay_disable_controller,
};

static int imxfb_alpha_set(struct param_d *param, void *priv)
{
	struct fb_info *overlay = priv;
	struct imxfb_info *fbi = overlay->priv;
	unsigned int tmp;

	if (fbi->alpha > 0xff)
		fbi->alpha = 0xff;

	tmp = readl(fbi->regs + LCDC_LGWCR);
	tmp &= ~LGWCR_GWAV(0xff);
	tmp |= LGWCR_GWAV(fbi->alpha);
	writel(tmp , fbi->regs + LCDC_LGWCR);

	return 0;
}

static int imxfb_register_overlay(struct imxfb_info *fbi, void *fb)
{
	struct fb_info *overlay;
	struct imxfb_rgb *rgb;
	int ret;

	overlay = &fbi->overlay;

	overlay->priv = fbi;
	overlay->mode = fbi->info.mode;
	overlay->xres = fbi->info.xres;
	overlay->yres = fbi->info.yres;
	overlay->bits_per_pixel = fbi->info.bits_per_pixel;
	overlay->fbops = &imxfb_overlay_ops;

	ret = imxfb_allocate_fbbuffer(fbi->dev, overlay, fb);
	if (ret < 0)
		return ret;

	writel((unsigned long)overlay->screen_base, fbi->regs + LCDC_LGWSAR);
	writel(SIZE_XMAX(overlay->xres) | SIZE_YMAX(overlay->yres),
			fbi->regs + LCDC_LGWSR);
	writel(VPW_VPW(overlay->xres * overlay->bits_per_pixel / 8 / 4),
		fbi->regs + LCDC_LGWVPWR);
	writel(0, fbi->regs + LCDC_LGWPR);
	writel(LGWCR_GWAV(0x0), fbi->regs + LCDC_LGWCR);

	switch (overlay->bits_per_pixel) {
	case 32:
		rgb = &def_rgb_18;
		break;
	case 16:
	default:
		if (fbi->pcr & PCR_TFT)
			rgb = &def_rgb_16_tft;
		else
			rgb = &def_rgb_16_stn;
		break;
	case 8:
		rgb = &def_rgb_8;
		break;
	}

	/*
	 * Copy the RGB parameters for this display
	 * from the machine specific parameters.
	 */
	overlay->red    = rgb->red;
	overlay->green  = rgb->green;
	overlay->blue   = rgb->blue;
	overlay->transp = rgb->transp;

	ret = register_framebuffer(overlay);
	if (ret < 0) {
		dev_err(fbi->dev, "failed to register framebuffer\n");
		return ret;
	}

	dev_add_param_uint32(&overlay->dev, "alpha", imxfb_alpha_set,
			NULL, &fbi->alpha, "%u", overlay);

	return 0;
}
#endif

static int imxfb_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imxfb_info *fbi;
	struct fb_info *info;
	struct imx_fb_platform_data *pdata = dev->platform_data;
	struct fb_videomode *mode_list;
	int ret, i;

	if (!pdata)
		return -ENODEV;

	if (!pdata->num_modes) {
		dev_err(dev, "no modes. bailing out\n");
		return -EINVAL;
	}

	mode_list = xzalloc(sizeof(*mode_list) * pdata->num_modes);
	for (i = 0; i < pdata->num_modes; i++)
		mode_list[i] = pdata->mode[i];

	fbi = xzalloc(sizeof(*fbi));
	info = &fbi->info;

	fbi->per_clk = clk_get(dev, "per");
	if (IS_ERR(fbi->per_clk))
		return PTR_ERR(fbi->per_clk);

	fbi->ahb_clk = clk_get(dev, "ahb");
	if (IS_ERR(fbi->ahb_clk))
		return PTR_ERR(fbi->ahb_clk);

	fbi->ipg_clk = clk_get(dev, "ipg");
	if (IS_ERR(fbi->ipg_clk))
		return PTR_ERR(fbi->ipg_clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	fbi->regs = IOMEM(iores->start);

	fbi->pcr = pdata->pcr;
	fbi->pwmr = pdata->pwmr;
	fbi->lscr1 = pdata->lscr1;
	fbi->dmacr = pdata->dmacr;
	fbi->enable = pdata->enable;
	fbi->dev = dev;
	info->priv = fbi;
	info->modes.modes = mode_list;
	info->modes.num_modes = pdata->num_modes;
	info->mode = pdata->mode;
	info->xres = pdata->mode->xres;
	info->yres = pdata->mode->yres;
	info->bits_per_pixel = pdata->bpp;
	info->fbops = &imxfb_ops;

	dev_info(dev, "i.MX Framebuffer driver\n");


	ret = imxfb_allocate_fbbuffer(dev, info, pdata->framebuffer);
	if (ret < 0)
		return ret;

	imxfb_activate_var(&fbi->info);

	ret = register_framebuffer(&fbi->info);
	if (ret < 0) {
		dev_err(dev, "failed to register framebuffer\n");
		return ret;
	}
#ifdef CONFIG_IMXFB_DRIVER_VIDEO_IMX_OVERLAY
	imxfb_register_overlay(fbi, pdata->framebuffer_ovl);
#endif
	return 0;
}

static struct driver_d imxfb_driver = {
	.name		= "imxfb",
	.probe		= imxfb_probe,
};
device_platform_driver(imxfb_driver);
