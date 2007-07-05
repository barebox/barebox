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

struct efb_info;

struct fb_data *s1d13706fb_init(struct efb_info *info);

/*
 * This structure describes the machine which we are running on.
 */
struct efb_info {
	struct fb_data	fbd;

	void		*regs;

	u_long		pixclock;

	u_char		hsync_len;
	u_char		left_margin;
	u_char		right_margin;

	u_char		vsync_len;
	u_char		upper_margin;
	u_char		lower_margin;
	u_char		sync;

	u_int		cmap_greyscale:1,
			unused:31;

	u_char		panel_type;

	int  (*init)(struct efb_info *);
};

#define EFB_RC		0x0

#define EFB_PCLK_CONF	0x05
#define PCLK_SOURCE_MCLK	0
#define PCLK_SOURCE_BCLK	1
#define PCLK_SOURCE_CLKI	2
#define PCLK_SOURCE_CLKI2	3

#define EFB_LUT_B_WRITE		0x08
#define EFB_LUT_G_WRITE		0x09
#define EFB_LUT_R_WRITE		0x0a
#define EFB_LUT_ADR_WRITE	0x0b

#define EFB_PANEL_TYPE		0x10
#define PANEL_TYPE_STN		0
#define PANEL_TYPE_TFT		1
#define PANEL_TYPE_HR_TFT	2
#define PANEL_TYPE_D_TFD	3
#define PANEL_TYPE_WIDTH_4	(0 << 4)
#define PANEL_TYPE_WIDTH_8	(1 << 4)
#define PANEL_TYPE_WIDTH_9	(0 << 4)
#define PANEL_TYPE_WIDTH_12	(1 << 4)
#define PANEL_TYPE_WIDTH_16	(2 << 4)
#define PANEL_TYPE_WIDTH_18	(2 << 4)
#define PANEL_TYPE_COLOR	(1 << 6)
#define PANEL_TYPE_FORMAT_2	(1 << 7)

#define EFB_HT			0x12
#define EFB_HDP			0x14
#define EFB_HDSP0		0x16
#define EFB_HDSP1		0x17
#define EFB_VT0			0x18
#define EFB_VT1			0x19
#define EFB_VDP0		0x1c
#define EFB_VDP1		0x1d
#define EFB_VDSP0		0x1e
#define EFB_VDSP1		0x1f
#define EFB_FPLINE_PULSE_WIDTH	0x20
#define FPLINE_ACT_HIGH	(1 << 7)
#define EFB_FPLINE_PULSE_START0	0x22
#define EFB_FPLINE_PULSE_START1	0x23
#define EFB_FPFRAME_PULSE_WIDTH	0x24
#define FPFRAME_ACT_HIGH	(1 << 7)

#define EFB_DISPLAY_MODE		0x70
#define DISPLAY_MODE_BPP_1		0
#define DISPLAY_MODE_BPP_2		1
#define DISPLAY_MODE_BPP_4		2
#define DISPLAY_MODE_BPP_8		3
#define DISPLAY_MODE_BPP_16		4

#define EFB_DISPLAY_START_ADDRESS0	0x74
#define EFB_DISPLAY_START_ADDRESS1	0x75
#define EFB_DISPLAY_START_ADDRESS2	0x76

#define EFB_LINE_ADDRESS_OFFSET0	0x78
#define EFB_LINE_ADDRESS_OFFSET1	0x79

#define EFB_POWER_SAVE_CONF		0xa0

#define EFB_GPIO_CONTROL0		0xac
#define EFB_GPIO_CONTROL1		0xad
#define GPIO_CONTROL0_GPO		(1 << 7)


