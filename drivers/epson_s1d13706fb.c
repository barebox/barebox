/*
 * (C) Copyright 2007 Pengutronix
 * Sascha Hauer, <s.hauer@pengutronix.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>

#ifdef CFG_S1D13706FB

#include <splash.h>
#include <s1d13706fb.h>
#include <asm/io.h>

#define panel_is_stn(panel_type) ((panel_type & 3) == PANEL_TYPE_STN ? 1 : 0)

static int efb_setcolreg(struct fb_data *fbd, int reg, int r, int g, int b)
{
	struct efb_info *info = fbd->priv;

	writeb(r & 0xfc, info->regs + EFB_LUT_R_WRITE);
	writeb(g & 0xfc, info->regs + EFB_LUT_G_WRITE);
	writeb(b & 0xfc, info->regs + EFB_LUT_B_WRITE);
	writeb(reg, info->regs + EFB_LUT_ADR_WRITE);

	return 0;
}

struct fb_data *s1d13706fb_init(struct efb_info *info)
{
	struct fb_data *fbd = &info->fbd;
	int tmp, mode, total;

	fbd->setcolreg = efb_setcolreg;
	fbd->priv = info;

	switch (fbd->bpp) {
	case 1:
		mode = DISPLAY_MODE_BPP_1;
		break;
	case 2:
		mode = DISPLAY_MODE_BPP_2;
		break;
	case 4:
		mode = DISPLAY_MODE_BPP_4;
		break;
	case 8:
		mode = DISPLAY_MODE_BPP_8;
		break;
	case 16:
	default:
		return NULL;
	}

	writeb(mode, info->regs + EFB_DISPLAY_MODE);

	total = (fbd->xres + info->left_margin + info->right_margin + info->hsync_len - 1) >> 3;

	writeb(total, info->regs + EFB_HT);
	writeb((fbd->xres >> 3) - 1, info->regs + EFB_HDP);


	if(panel_is_stn(info->panel_type))
		tmp = info->left_margin - 22;
	else
		tmp = info->left_margin - 5;

	writeb(tmp & 0xff, info->regs + EFB_HDSP0);
	writeb((tmp >> 8) & 0x3, info->regs + EFB_HDSP1);

	total = fbd->yres + info->upper_margin + info->lower_margin + info->vsync_len - 1;

	writeb(total & 0xff, info->regs + EFB_VT0);
	writeb((total >> 8) & 0xff, info->regs + EFB_VT1);

	writeb((fbd->yres - 1) & 0xff, info->regs + EFB_VDP0);
	writeb(((fbd->yres - 1) >> 8) & 0xff, info->regs + EFB_VDP1);

	writeb(info->upper_margin & 0xff, info->regs + EFB_VDSP0);
	writeb((info->upper_margin >> 8) & 0xff, info->regs + EFB_VDSP1);

	writeb(info->panel_type, info->regs + EFB_PANEL_TYPE);

	writeb( ((info->sync & FB_SYNC_HOR_HIGH_ACT) ? FPLINE_ACT_HIGH : 0) |
		(info->hsync_len -1), info->regs + EFB_FPLINE_PULSE_WIDTH);

	writeb( ((info->sync & FB_SYNC_VERT_HIGH_ACT) ? FPFRAME_ACT_HIGH : 0) |
		(info->vsync_len - 1), info->regs + EFB_FPFRAME_PULSE_WIDTH);

	tmp = (fbd->xres * fbd->bpp) >> 3;
	writeb( (tmp >> 2) & 0xff, info->regs + EFB_LINE_ADDRESS_OFFSET0);
	writeb( (tmp >> 10) & 0x3, info->regs + EFB_LINE_ADDRESS_OFFSET1);

	/* erase framebuffer */
	memset(fbd->fb, 0, 80 * 1024);

	efb_setcolreg(fbd, 0, 0, 0, 0);

	if(info->init)
		info->init(info);

	writeb(0, info->regs + EFB_POWER_SAVE_CONF); /* enable controller */

	return fbd;
}

#endif /* CFG_S1D13706FB */

