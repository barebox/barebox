/*
 * Copyright (C) 2009
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
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
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/imx35-regs.h>
#include <fb.h>
#include <mach/imxfb.h>
#include <malloc.h>
#include <errno.h>
#include <asm-generic/div64.h>
#include <mach/imx-ipu-fb.h>
#include <linux/clk.h>
#include <linux/err.h>

struct ipu_fb_info {
	void __iomem		*regs;
	struct clk		*clk;

	void			(*enable)(int enable);

	struct fb_info		info;
	struct fb_info		overlay;
	struct device_d		*dev;

	unsigned int		alpha;
};

/* IPU DMA Controller channel definitions. */
enum ipu_channel {
	IDMAC_IC_0 = 0,		/* IC (encoding task) to memory */
	IDMAC_IC_1 = 1,		/* IC (viewfinder task) to memory */
	IDMAC_ADC_0 = 1,
	IDMAC_IC_2 = 2,
	IDMAC_ADC_1 = 2,
	IDMAC_IC_3 = 3,
	IDMAC_IC_4 = 4,
	IDMAC_IC_5 = 5,
	IDMAC_IC_6 = 6,
	IDMAC_IC_7 = 7,		/* IC (sensor data) to memory */
	IDMAC_IC_8 = 8,
	IDMAC_IC_9 = 9,
	IDMAC_IC_10 = 10,
	IDMAC_IC_11 = 11,
	IDMAC_IC_12 = 12,
	IDMAC_IC_13 = 13,
	IDMAC_SDC_0 = 14,	/* Background synchronous display data */
	IDMAC_SDC_1 = 15,	/* Foreground data (overlay) */
	IDMAC_SDC_2 = 16,
	IDMAC_SDC_3 = 17,
	IDMAC_ADC_2 = 18,
	IDMAC_ADC_3 = 19,
	IDMAC_ADC_4 = 20,
	IDMAC_ADC_5 = 21,
	IDMAC_ADC_6 = 22,
	IDMAC_ADC_7 = 23,
	IDMAC_PF_0 = 24,
	IDMAC_PF_1 = 25,
	IDMAC_PF_2 = 26,
	IDMAC_PF_3 = 27,
	IDMAC_PF_4 = 28,
	IDMAC_PF_5 = 29,
	IDMAC_PF_6 = 30,
	IDMAC_PF_7 = 31,
};

/* More formats can be copied from the Linux driver if needed */
enum pixel_fmt {
	/* 2 bytes */
	IPU_PIX_FMT_RGB565,
	IPU_PIX_FMT_RGB666,
	IPU_PIX_FMT_BGR666,
	/* 3 bytes */
	IPU_PIX_FMT_RGB24,
};

struct pixel_fmt_cfg {
	u32	b0;
	u32	b1;
	u32	b2;
	u32	acc;
};

static struct pixel_fmt_cfg fmt_cfg[] = {
	[IPU_PIX_FMT_RGB24] = {
		0x1600AAAA, 0x00E05555, 0x00070000, 3,
	},
	[IPU_PIX_FMT_RGB666] = {
		0x0005000F, 0x000B000F, 0x0011000F, 1,
	},
	[IPU_PIX_FMT_BGR666] = {
		0x0011000F, 0x000B000F, 0x0005000F, 1,
	},
	[IPU_PIX_FMT_RGB565] = {
		0x0004003F, 0x000A000F, 0x000F003F, 1,
	}
};

enum ipu_panel {
	IPU_PANEL_SHARP_TFT,
	IPU_PANEL_TFT,
};

/* IPU Common registers */
#define IPU_CONF		0x00
#define IPU_CHA_BUF0_RDY	0x04
#define IPU_CHA_BUF1_RDY	0x08
#define IPU_CHA_DB_MODE_SEL	0x0C
#define IPU_CHA_CUR_BUF		0x10
#define IPU_FS_PROC_FLOW	0x14
#define IPU_FS_DISP_FLOW	0x18
#define IPU_TASKS_STAT		0x1C
#define IPU_IMA_ADDR		0x20
#define IPU_IMA_DATA		0x24
#define IPU_INT_CTRL_1		0x28
#define IPU_INT_CTRL_2		0x2C
#define IPU_INT_CTRL_3		0x30
#define IPU_INT_CTRL_4		0x34
#define IPU_INT_CTRL_5		0x38
#define IPU_INT_STAT_1		0x3C
#define IPU_INT_STAT_2		0x40
#define IPU_INT_STAT_3		0x44
#define IPU_INT_STAT_4		0x48
#define IPU_INT_STAT_5		0x4C
#define IPU_BRK_CTRL_1		0x50
#define IPU_BRK_CTRL_2		0x54
#define IPU_BRK_STAT		0x58
#define IPU_DIAGB_CTRL		0x5C

#define IPU_CONF_PXL_ENDIAN	(1<<8)
#define IPU_CONF_DU_EN		(1<<7)
#define IPU_CONF_DI_EN		(1<<6)
#define IPU_CONF_ADC_EN		(1<<5)
#define IPU_CONF_SDC_EN		(1<<4)
#define IPU_CONF_PF_EN		(1<<3)
#define IPU_CONF_ROT_EN		(1<<2)
#define IPU_CONF_IC_EN		(1<<1)
#define IPU_CONF_SCI_EN		(1<<0)

/* Image Converter Registers */
#define IC_CONF			0x88
#define IC_PRP_ENC_RSC		0x8C
#define IC_PRP_VF_RSC		0x90
#define IC_PP_RSC		0x94
#define IC_CMBP_1		0x98
#define IC_CMBP_2		0x9C
#define PF_CONF			0xA0
#define IDMAC_CONF		0xA4
#define IDMAC_CHA_EN		0xA8
#define IDMAC_CHA_PRI		0xAC
#define IDMAC_CHA_BUSY		0xB0

/* Image Converter Register bits */
#define IC_CONF_PRPENC_EN	0x00000001
#define IC_CONF_PRPENC_CSC1	0x00000002
#define IC_CONF_PRPENC_ROT_EN	0x00000004
#define IC_CONF_PRPVF_EN	0x00000100
#define IC_CONF_PRPVF_CSC1	0x00000200
#define IC_CONF_PRPVF_CSC2	0x00000400
#define IC_CONF_PRPVF_CMB	0x00000800
#define IC_CONF_PRPVF_ROT_EN	0x00001000
#define IC_CONF_PP_EN		0x00010000
#define IC_CONF_PP_CSC1		0x00020000
#define IC_CONF_PP_CSC2		0x00040000
#define IC_CONF_PP_CMB		0x00080000
#define IC_CONF_PP_ROT_EN	0x00100000
#define IC_CONF_IC_GLB_LOC_A	0x10000000
#define IC_CONF_KEY_COLOR_EN	0x20000000
#define IC_CONF_RWS_EN		0x40000000
#define IC_CONF_CSI_MEM_WR_EN	0x80000000

/* SDC Registers */
#define SDC_COM_CONF		0xB4
#define SDC_GW_CTRL		0xB8
#define SDC_FG_POS		0xBC
#define SDC_BG_POS		0xC0
#define SDC_CUR_POS		0xC4
#define SDC_PWM_CTRL		0xC8
#define SDC_CUR_MAP		0xCC
#define SDC_HOR_CONF		0xD0
#define SDC_VER_CONF		0xD4
#define SDC_SHARP_CONF_1	0xD8
#define SDC_SHARP_CONF_2	0xDC

/* Register bits */
#define SDC_COM_TFT_COLOR	0x00000001UL
#define SDC_COM_FG_EN		0x00000010UL
#define SDC_COM_GWSEL		0x00000020UL
#define SDC_COM_GLB_A		0x00000040UL
#define SDC_COM_KEY_COLOR_G	0x00000080UL
#define SDC_COM_BG_EN		0x00000200UL
#define SDC_COM_SHARP		0x00001000UL

#define SDC_V_SYNC_WIDTH_L	0x00000001UL

/* Display Interface registers */
#define DI_DISP_IF_CONF		0x0124
#define DI_DISP_SIG_POL		0x0128
#define DI_SER_DISP1_CONF	0x012C
#define DI_SER_DISP2_CONF	0x0130
#define DI_HSP_CLK_PER		0x0134
#define DI_DISP0_TIME_CONF_1	0x0138
#define DI_DISP0_TIME_CONF_2	0x013C
#define DI_DISP0_TIME_CONF_3	0x0140
#define DI_DISP1_TIME_CONF_1	0x0144
#define DI_DISP1_TIME_CONF_2	0x0148
#define DI_DISP1_TIME_CONF_3	0x014C
#define DI_DISP2_TIME_CONF_1	0x0150
#define DI_DISP2_TIME_CONF_2	0x0154
#define DI_DISP2_TIME_CONF_3	0x0158
#define DI_DISP3_TIME_CONF	0x015C
#define DI_DISP0_DB0_MAP	0x0160
#define DI_DISP0_DB1_MAP	0x0164
#define DI_DISP0_DB2_MAP	0x0168
#define DI_DISP0_CB0_MAP	0x016C
#define DI_DISP0_CB1_MAP	0x0170
#define DI_DISP0_CB2_MAP	0x0174
#define DI_DISP1_DB0_MAP	0x0178
#define DI_DISP1_DB1_MAP	0x017C
#define DI_DISP1_DB2_MAP	0x0180
#define DI_DISP1_CB0_MAP	0x0184
#define DI_DISP1_CB1_MAP	0x0188
#define DI_DISP1_CB2_MAP	0x018C
#define DI_DISP2_DB0_MAP	0x0190
#define DI_DISP2_DB1_MAP	0x0194
#define DI_DISP2_DB2_MAP	0x0198
#define DI_DISP2_CB0_MAP	0x019C
#define DI_DISP2_CB1_MAP	0x01A0
#define DI_DISP2_CB2_MAP	0x01A4
#define DI_DISP3_B0_MAP		0x01A8
#define DI_DISP3_B1_MAP		0x01AC
#define DI_DISP3_B2_MAP		0x01B0
#define DI_DISP_ACC_CC		0x01B4
#define DI_DISP_LLA_CONF	0x01B8
#define DI_DISP_LLA_DATA	0x01BC

/* DI_DISP_SIG_POL bits */
#define DI_D3_VSYNC_POL		(1 << 28)
#define DI_D3_HSYNC_POL		(1 << 27)
#define DI_D3_DRDY_SHARP_POL	(1 << 26)
#define DI_D3_CLK_POL		(1 << 25)
#define DI_D3_DATA_POL		(1 << 24)

/* DI_DISP_IF_CONF bits */
#define DI_D3_CLK_IDLE		(1 << 26)
#define DI_D3_CLK_SEL		(1 << 25)
#define DI_D3_DATAMSK		(1 << 24)

struct imx_ipu_fb_rgb {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

static struct imx_ipu_fb_rgb def_rgb_16 = {
	.red	= {.offset = 11, .length = 5,},
	.green	= {.offset = 5, .length = 6,},
	.blue	= {.offset = 0, .length = 5,},
	.transp = {.offset = 0, .length = 0,},
};

static struct imx_ipu_fb_rgb def_rgb_24 = {
	.red	= {.offset = 16, .length = 8,},
	.green	= {.offset = 8, .length = 8,},
	.blue	= {.offset = 0, .length = 8,},
	.transp = {.offset = 0, .length = 0,},
};

static struct imx_ipu_fb_rgb def_rgb_32 = {
	.red	= {.offset = 16, .length = 8,},
	.green	= {.offset = 8, .length = 8,},
	.blue	= {.offset = 0, .length = 8,},
	.transp = {.offset = 24, .length = 8,},
};

struct chan_param_mem_planar {
	/* Word 0 */
	u32	xv:10;
	u32	yv:10;
	u32	xb:12;

	u32	yb:12;
	u32	res1:2;
	u32	nsb:1;
	u32	lnpb:6;
	u32	ubo_l:11;

	u32	ubo_h:15;
	u32	vbo_l:17;

	u32	vbo_h:9;
	u32	res2:3;
	u32	fw:12;
	u32	fh_l:8;

	u32	fh_h:4;
	u32	res3:28;

	/* Word 1 */
	u32	eba0;

	u32	eba1;

	u32	bpp:3;
	u32	sl:14;
	u32	pfs:3;
	u32	bam:3;
	u32	res4:2;
	u32	npb:6;
	u32	res5:1;

	u32	sat:2;
	u32	res6:30;
} __attribute__ ((packed));

struct chan_param_mem_interleaved {
	/* Word 0 */
	u32	xv:10;
	u32	yv:10;
	u32	xb:12;

	u32	yb:12;
	u32	sce:1;
	u32	res1:1;
	u32	nsb:1;
	u32	lnpb:6;
	u32	sx:10;
	u32	sy_l:1;

	u32	sy_h:9;
	u32	ns:10;
	u32	sm:10;
	u32	sdx_l:3;

	u32	sdx_h:2;
	u32	sdy:5;
	u32	sdrx:1;
	u32	sdry:1;
	u32	sdr1:1;
	u32	res2:2;
	u32	fw:12;
	u32	fh_l:8;

	u32	fh_h:4;
	u32	res3:28;

	/* Word 1 */
	u32	eba0;

	u32	eba1;

	u32	bpp:3;
	u32	sl:14;
	u32	pfs:3;
	u32	bam:3;
	u32	res4:2;
	u32	npb:6;
	u32	res5:1;

	u32	sat:2;
	u32	scc:1;
	u32	ofs0:5;
	u32	ofs1:5;
	u32	ofs2:5;
	u32	ofs3:5;
	u32	wid0:3;
	u32	wid1:3;
	u32	wid2:3;

	u32	wid3:3;
	u32	dec_sel:1;
	u32	res6:28;
} __attribute__ ((packed));

union chan_param_mem {
	struct chan_param_mem_planar		pp;
	struct chan_param_mem_interleaved	ip;
};

static inline u32 reg_read(struct ipu_fb_info *fbi, unsigned long reg)
{
	u32 val;

	val = readl(fbi->regs + reg);

	debug("%s: %p 0x%08x\n", __func__, fbi->regs + reg, val);

	return val;
}

static inline void reg_write(struct ipu_fb_info *fbi, u32 value,
		unsigned long reg)
{
	debug("%s: %p 0x%08x\n", __func__, fbi->regs + reg, value);

	writel(value, fbi->regs + reg);
}

/*
 * sdc_init_panel() - initialize a synchronous LCD panel.
 * @width:		width of panel in pixels.
 * @height:		height of panel in pixels.
 * @pixel_fmt:		pixel format of buffer as FOURCC ASCII code.
 * @return:		0 on success or negative error code on failure.
 */
static int sdc_init_panel(struct fb_info *info, enum pixel_fmt pixel_fmt)
{
	struct ipu_fb_info *fbi = info->priv;
	struct fb_videomode *mode = info->mode;
	u32 reg, old_conf, div;
	enum ipu_panel panel = IPU_PANEL_TFT;
	unsigned long pixel_clk;

	/* Init panel size and blanking periods */
	reg = ((mode->hsync_len - 1) << 26) |
		((info->xres + mode->left_margin + mode->right_margin +
		  mode->hsync_len - 1) << 16);
	reg_write(fbi, reg, SDC_HOR_CONF);

	reg = ((mode->vsync_len - 1) << 26) | SDC_V_SYNC_WIDTH_L |
		((info->yres + mode->upper_margin + mode->lower_margin +
		  mode->vsync_len - 1) << 16);
	reg_write(fbi, reg, SDC_VER_CONF);

	old_conf = reg_read(fbi, DI_DISP_SIG_POL) & 0xE0FFFFFF;
	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		old_conf |= DI_D3_HSYNC_POL;
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		old_conf |= DI_D3_VSYNC_POL;
	if (mode->sync & FB_SYNC_CLK_INVERT)
		old_conf |= DI_D3_CLK_POL;
	if (mode->sync & FB_SYNC_DATA_INVERT)
		old_conf |= DI_D3_DATA_POL;
	if (mode->sync & FB_SYNC_OE_ACT_HIGH)
		old_conf |= DI_D3_DRDY_SHARP_POL;
	reg_write(fbi, old_conf, DI_DISP_SIG_POL);

	old_conf = reg_read(fbi, DI_DISP_IF_CONF) & 0x78FFFFFF;
	if (mode->sync & FB_SYNC_CLK_IDLE_EN)
		old_conf |= DI_D3_CLK_IDLE;
	if (mode->sync & FB_SYNC_CLK_SEL_EN)
		old_conf |= DI_D3_CLK_SEL;
	if (mode->sync & FB_SYNC_SHARP_MODE)
		panel = IPU_PANEL_SHARP_TFT;
	reg_write(fbi, old_conf, DI_DISP_IF_CONF);

	switch (panel) {
	case IPU_PANEL_SHARP_TFT:
		reg_write(fbi, 0x00FD0102L, SDC_SHARP_CONF_1);
		reg_write(fbi, 0x00F500F4L, SDC_SHARP_CONF_2);
		reg = reg_read(fbi, SDC_COM_CONF);
		reg_write(fbi, reg | SDC_COM_SHARP | SDC_COM_TFT_COLOR,
							SDC_COM_CONF);
		break;
	case IPU_PANEL_TFT:
		reg = reg_read(fbi, SDC_COM_CONF) & ~SDC_COM_SHARP;
		reg_write(fbi, reg | SDC_COM_TFT_COLOR, SDC_COM_CONF);
		break;
	default:
		return -EINVAL;
	}

	/*
	 * Calculate divider: fractional part is 4 bits so simply multiple by
	 * 2^4 to get fractional part, as long as we stay under ~250MHz and on
	 * i.MX31 it (HSP_CLK) is <= 178MHz. Currently 128.267MHz
	 */
	pixel_clk = PICOS2KHZ(mode->pixclock) * 1000UL;
	div = clk_get_rate(fbi->clk) * 16 / pixel_clk;

	if (div < 0x40) {	/* Divider less than 4 */
		dev_dbg(&info->dev,
			"InitPanel() - Pixel clock divider less than 4\n");
		div = 0x40;
	}

	dev_dbg(&info->dev, "pixel clk = %lu, divider %u.%u\n",
		pixel_clk, div >> 4, (div & 7) * 125);

	/*
	 * DISP3_IF_CLK_DOWN_WR is half the divider value and 2 fraction bits
	 * fewer. Subtract 1 extra from DISP3_IF_CLK_DOWN_WR based on timing
	 * debug. DISP3_IF_CLK_UP_WR is 0
	 */
	reg_write(fbi, (((div / 8) - 1) << 22) | div, DI_DISP3_TIME_CONF);

	reg_write(fbi, fmt_cfg[pixel_fmt].b0, DI_DISP3_B0_MAP);
	reg_write(fbi, fmt_cfg[pixel_fmt].b1, DI_DISP3_B1_MAP);
	reg_write(fbi, fmt_cfg[pixel_fmt].b2, DI_DISP3_B2_MAP);
	reg_write(fbi, reg_read(fbi, DI_DISP_ACC_CC) |
		  ((fmt_cfg[pixel_fmt].acc - 1) << 12), DI_DISP_ACC_CC);

	return 0;
}

static void ipu_ch_param_set_size(union chan_param_mem *params,
				  u32 pixel_fmt, uint16_t width,
				  uint16_t height, uint16_t stride)
{
	params->pp.fw		= width - 1;
	params->pp.fh_l		= height - 1;
	params->pp.fh_h		= (height - 1) >> 8;
	params->pp.sl		= stride - 1;

	/* See above, for further formats see the Linux driver */
	switch (pixel_fmt) {
	case IPU_PIX_FMT_RGB565:
		params->ip.bpp	= 2;
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 0;		/* Red bit offset */
		params->ip.ofs1	= 5;		/* Green bit offset */
		params->ip.ofs2	= 11;		/* Blue bit offset */
		params->ip.ofs3	= 16;		/* Alpha bit offset */
		params->ip.wid0	= 4;		/* Red bit width - 1 */
		params->ip.wid1	= 5;		/* Green bit width - 1 */
		params->ip.wid2	= 4;		/* Blue bit width - 1 */
		break;
	case IPU_PIX_FMT_RGB24:
		params->ip.bpp	= 1;		/* 24 BPP & RGB PFS */
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 16;		/* Red bit offset */
		params->ip.ofs1	= 8;		/* Green bit offset */
		params->ip.ofs2	= 0;		/* Blue bit offset */
		params->ip.ofs3	= 24;		/* Alpha bit offset */
		params->ip.wid0	= 7;		/* Red bit width - 1 */
		params->ip.wid1	= 7;		/* Green bit width - 1 */
		params->ip.wid2	= 7;		/* Blue bit width - 1 */
		break;
	default:
		break;
	}

	params->pp.nsb = 1;
}

static void ipu_ch_param_set_buffer(union chan_param_mem *params,
				    void *buf0, void *buf1)
{
	params->pp.eba0 = (u32)buf0;
	params->pp.eba1 = (u32)buf1;
}

static void ipu_write_param_mem(struct ipu_fb_info *fbi, u32 addr,
		u32 *data, u32 num_words)
{
	for (; num_words > 0; num_words--) {
		reg_write(fbi, addr, IPU_IMA_ADDR);
		reg_write(fbi, *data++, IPU_IMA_DATA);
		addr++;
		if ((addr & 0x7) == 5) {
			addr &= ~0x7;	/* set to word 0 */
			addr += 8;	/* increment to next row */
		}
	}
}

static u32 bpp_to_pixfmt(int bpp)
{
	switch (bpp) {
	case 16:
		return IPU_PIX_FMT_RGB565;
	default:
		return 0;
	}
}

static u32 dma_param_addr(enum ipu_channel channel)
{
	/* Channel Parameter Memory */
	return 0x10000 | (channel << 4);
}

static void ipu_init_channel_buffer(struct ipu_fb_info *fbi,
		enum ipu_channel channel, void *fbmem)
{
	union chan_param_mem params = {};
	u32 reg;
	u32 stride_bytes;

	stride_bytes = fbi->info.xres * ((fbi->info.bits_per_pixel + 7) / 8);
	stride_bytes = (stride_bytes + 3) & ~3;

	/* Build parameter memory data for DMA channel */
	ipu_ch_param_set_size(&params, bpp_to_pixfmt(fbi->info.bits_per_pixel),
			      fbi->info.xres, fbi->info.yres, stride_bytes);
	ipu_ch_param_set_buffer(&params, fbmem, NULL);
	params.pp.bam = 0;
	/* Some channels (rotation) have restriction on burst length */

	switch (channel) {
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
		/* In original code only IPU_PIX_FMT_RGB565 was setting burst */
		params.pp.npb = 16 - 1;
		break;
	default:
		break;
	}

	ipu_write_param_mem(fbi, dma_param_addr(channel), (u32 *)&params, 10);

	/* Disable double-buffering */
	reg = reg_read(fbi, IPU_CHA_DB_MODE_SEL);
	reg &= ~(1UL << channel);
	reg_write(fbi, reg, IPU_CHA_DB_MODE_SEL);
}

static void ipu_channel_set_priority(struct ipu_fb_info *fbi,
		enum ipu_channel channel, int prio)
{
	u32 reg;

	reg = reg_read(fbi, IDMAC_CHA_PRI);

	if (prio)
		reg |= 1UL << channel;
	else
		reg &= ~(1UL << channel);

	reg_write(fbi, reg, IDMAC_CHA_PRI);
}

/*
 * ipu_enable_channel() - enable an IPU channel.
 * @channel:	channel ID.
 * @return:	0 on success or negative error code on failure.
 */
static int ipu_enable_channel(struct ipu_fb_info *fbi, enum ipu_channel channel)
{
	u32 reg;

	/* Reset to buffer 0 */
	reg_write(fbi, 1UL << channel, IPU_CHA_CUR_BUF);

	switch (channel) {
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
		ipu_channel_set_priority(fbi, channel, 1);
		break;
	default:
		break;
	}

	reg = reg_read(fbi, IDMAC_CHA_EN);
	reg_write(fbi, reg | (1UL << channel), IDMAC_CHA_EN);

	return 0;
}

static int ipu_update_channel_buffer(struct ipu_fb_info *fbi,
		enum ipu_channel channel, void *buf)
{
	u32 reg;

	reg = reg_read(fbi, IPU_CHA_BUF0_RDY);
	if (reg & (1UL << channel))
		return -EACCES;

	/* 44.3.3.1.9 - Row Number 1 (WORD1, offset 0) */
	reg_write(fbi, dma_param_addr(channel) + 0x0008UL, IPU_IMA_ADDR);
	reg_write(fbi, (u32)buf, IPU_IMA_DATA);

	return 0;
}

static int idmac_tx_submit(struct ipu_fb_info *fbi, enum ipu_channel channel,
		void *buf)
{
	int ret;

	ipu_init_channel_buffer(fbi, channel, buf);


	/* ipu_idmac.c::ipu_submit_channel_buffers() */
	ret = ipu_update_channel_buffer(fbi, channel, buf);
	if (ret < 0)
		return ret;

	/* ipu_idmac.c::ipu_select_buffer() */
	/* Mark buffer 0 as ready. */
	reg_write(fbi, 1UL << channel, IPU_CHA_BUF0_RDY);


	ret = ipu_enable_channel(fbi, channel);
	return ret;
}

static void sdc_enable_channel(struct ipu_fb_info *fbi, void *fbmem,
				enum ipu_channel channel)
{
	int ret = 0;
	u32 reg;

	ret = idmac_tx_submit(fbi, channel, fbmem);

	/* mx3fb.c::sdc_fb_init() */
	if (ret >= 0) {
		reg = reg_read(fbi, SDC_COM_CONF);
		if (channel == IDMAC_SDC_1)
			reg_write(fbi, reg | SDC_COM_FG_EN, SDC_COM_CONF);
		else
			reg_write(fbi, reg | SDC_COM_BG_EN, SDC_COM_CONF);
	}

	/*
	 * Attention! Without this mdelay the channel keeps generating
	 * interrupts. Next sdc_set_brightness() is going to be called
	 * from mx3fb_blank().
	 */
	mdelay(2);
}

/* References in this function refer to respective Linux kernel sources */
static void ipu_fb_enable(struct fb_info *info)
{
	struct ipu_fb_info *fbi = info->priv;
	struct fb_videomode *mode = info->mode;
	u32 reg;

	/* pcm037.c::mxc_board_init() */

	/* ipu_idmac.c::ipu_probe() */

	/* Start the clock */
	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);
	reg |= (3 << 18);
	writel(reg, MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);

	/* ipu_idmac.c::ipu_idmac_init() */

	/* Service request counter to maximum - shouldn't be needed */
	reg_write(fbi, 0x00000070, IDMAC_CONF);

	/* ipu_idmac.c::ipu_init_channel() */

	/* Enable IPU sub modules */
	reg = reg_read(fbi, IPU_CONF) | IPU_CONF_SDC_EN | IPU_CONF_DI_EN;
	reg_write(fbi, reg, IPU_CONF);

	/* mx3fb.c::init_fb_chan() */

	/* set Display Interface clock period */
	reg_write(fbi, 0x00100010L, DI_HSP_CLK_PER);
	/* Might need to trigger HSP clock change - see 44.3.3.8.5 */

	/* mx3fb.c::sdc_set_brightness() */

	/* This might be board-specific */
	reg_write(fbi, 0x03000000UL | 255 << 16, SDC_PWM_CTRL);

	/* mx3fb.c::sdc_set_global_alpha() */

	/* Use global - not per-pixel - Alpha-blending */
	reg = reg_read(fbi, SDC_GW_CTRL) & 0x00FFFFFFL;
	reg_write(fbi, reg | ((u32) 0xff << 24), SDC_GW_CTRL);

	reg = reg_read(fbi, SDC_COM_CONF);
	reg_write(fbi, reg | SDC_COM_GLB_A, SDC_COM_CONF);

	/* mx3fb.c::sdc_set_color_key() */

	/* Disable colour-keying for background */
	reg = reg_read(fbi, SDC_COM_CONF) &
		~(SDC_COM_KEY_COLOR_G);
	reg_write(fbi, reg, SDC_COM_CONF);

	sdc_init_panel(info, IPU_PIX_FMT_RGB666);

	reg_write(fbi, (mode->left_margin << 16) | mode->upper_margin,
			SDC_BG_POS);

	sdc_enable_channel(fbi, info->screen_base, IDMAC_SDC_0);

	/*
	 * Linux driver calls sdc_set_brightness() here again,
	 * once is enough for us
	 */
	if (fbi->enable)
		fbi->enable(1);
}

static void ipu_fb_disable(struct fb_info *info)
{
	struct ipu_fb_info *fbi = info->priv;
	u32 reg;

	if (fbi->enable)
		fbi->enable(0);

	reg = reg_read(fbi, SDC_COM_CONF);
	reg &= ~SDC_COM_BG_EN;
	reg_write(fbi, reg, SDC_COM_CONF);
}

static int ipu_fb_activate_var(struct fb_info *info)
{
#ifdef CONFIG_DRIVER_VIDEO_IMX_IPU_OVERLAY
	struct ipu_fb_info *fbi = info->priv;
	struct fb_info *overlay = &fbi->overlay;

	/* overlay also needs to know the new values */
	overlay->mode = info->mode;
	overlay->xres = info->xres;
	overlay->yres = info->yres;
#endif

	return 0;
}

static struct fb_ops imxfb_ops = {
	.fb_enable = ipu_fb_enable,
	.fb_disable = ipu_fb_disable,
	.fb_activate_var = ipu_fb_activate_var,
};

static void imxfb_init_info(struct fb_info *info, struct fb_videomode *mode,
		int bpp)
{
	struct imx_ipu_fb_rgb *rgb;

	info->mode = mode;
	info->xres = mode->xres;
	info->yres = mode->yres;
	info->bits_per_pixel = bpp;

	switch (info->bits_per_pixel) {
	case 32:
		rgb = &def_rgb_32;
		break;
	case 24:
		rgb = &def_rgb_24;
		break;
	case 16:
	default:
		rgb = &def_rgb_16;
		break;
	}

	/*
	 * Copy the RGB parameters for this display
	 * from the machine specific parameters.
	 */
	info->red    = rgb->red;
	info->green  = rgb->green;
	info->blue   = rgb->blue;
	info->transp = rgb->transp;
}

#ifdef CONFIG_DRIVER_VIDEO_IMX_IPU_OVERLAY

static void ipu_fb_overlay_enable_controller(struct fb_info *overlay)
{
	struct ipu_fb_info *fbi = overlay->priv;
	struct fb_videomode *mode = overlay->mode;
	int reg;

	sdc_init_panel(overlay, IPU_PIX_FMT_RGB666);

	reg_write(fbi, (mode->left_margin << 16) | mode->upper_margin,
							SDC_FG_POS);

	reg = reg_read(fbi, SDC_COM_CONF);
	reg_write(fbi, reg | SDC_COM_GWSEL, SDC_COM_CONF);

	if (fbi->enable)
		fbi->enable(1);

	sdc_enable_channel(fbi, overlay->screen_base, IDMAC_SDC_1);
}

static void ipu_fb_overlay_disable_controller(struct fb_info *overlay)
{
	struct ipu_fb_info *fbi = overlay->priv;
	u32 reg;

	if (fbi->enable)
		fbi->enable(0);

	/* Disable foreground and set graphic window to background */
	reg = reg_read(fbi, SDC_COM_CONF);
	reg &= ~(SDC_COM_FG_EN | SDC_COM_GWSEL);
	reg_write(fbi, reg, SDC_COM_CONF);
}

static int ipu_fb_overlay_setcolreg(u_int regno, u_int red, u_int green,
		u_int blue, u_int trans, struct fb_info *info)
{
	return 0;
}

static struct fb_ops ipu_fb_overlay_ops = {
	.fb_setcolreg	= ipu_fb_overlay_setcolreg,
	.fb_enable	= ipu_fb_overlay_enable_controller,
	.fb_disable	= ipu_fb_overlay_disable_controller,
};

static int sdc_alpha_set(struct param_d *param, void *priv)
{
	struct fb_info *info = priv;
	struct ipu_fb_info *fbi = info->priv;
	unsigned int tmp;

	if (fbi->alpha > 0xff)
		fbi->alpha = 0xff;

	tmp = reg_read(fbi, SDC_GW_CTRL) & 0x00FFFFFFL;
	reg_write(fbi, tmp | ((u32) fbi->alpha << 24), SDC_GW_CTRL);

	return 0;
}

static int sdc_fb_register_overlay(struct ipu_fb_info *fbi, void *fb)
{
	struct fb_info *overlay;
	const struct imx_ipu_fb_platform_data *pdata = fbi->dev->platform_data;
	int ret;

	overlay = &fbi->overlay;
	overlay->priv = fbi;
	overlay->fbops = &ipu_fb_overlay_ops;

	imxfb_init_info(overlay, pdata->mode, pdata->bpp);

	if (fb)
		overlay->screen_base = fb;
	else
		overlay->screen_base = xzalloc(overlay->xres * overlay->yres *
				(overlay->bits_per_pixel >> 3));

	if (!overlay->screen_base)
		return -ENOMEM;

	sdc_enable_channel(fbi, overlay->screen_base, IDMAC_SDC_1);

	ret = register_framebuffer(&fbi->overlay);
	if (ret < 0) {
		dev_err(fbi->dev, "failed to register framebuffer\n");
		return ret;
	}

	dev_add_param_int(&overlay->dev, "alpha", sdc_alpha_set,
			NULL, &fbi->alpha, "%u", overlay);

	return 0;
}

#endif

static int imxfb_probe(struct device_d *dev)
{
	struct ipu_fb_info *fbi;
	struct fb_info *info;
	const struct imx_ipu_fb_platform_data *pdata = dev->platform_data;
	int ret = 0;

	if (!pdata)
		return -ENODEV;

	fbi = xzalloc(sizeof(*fbi));
	info = &fbi->info;

	fbi->clk = clk_get(dev, NULL);
	if (IS_ERR(fbi->clk))
		return PTR_ERR(fbi->clk);

	fbi->regs = dev_request_mem_region(dev, 0);
	fbi->dev = dev;
	fbi->enable = pdata->enable;
	info->priv = fbi;
	info->fbops = &imxfb_ops;
	info->num_modes = pdata->num_modes;
	info->mode_list = pdata->mode;

	imxfb_init_info(info, pdata->mode, pdata->bpp);

	dev_info(dev, "i.MX Framebuffer driver\n");

	/*
	 * Use a given frambuffer or reserve some
	 * memory for screen usage
	 */
	fbi->info.screen_base = pdata->framebuffer;
	if (fbi->info.screen_base == NULL) {
		fbi->info.screen_base = malloc(info->xres * info->yres *
					       (info->bits_per_pixel >> 3));
		if (!fbi->info.screen_base)
			return -ENOMEM;
	}

	sdc_enable_channel(fbi, info->screen_base, IDMAC_SDC_0);

	ret = register_framebuffer(&fbi->info);
	if (ret < 0) {
		dev_err(dev, "failed to register framebuffer\n");
		return ret;
	}

#ifdef CONFIG_DRIVER_VIDEO_IMX_IPU_OVERLAY
	ret = sdc_fb_register_overlay(fbi, pdata->framebuffer_ovl);
#endif
	return ret;
}

static void imxfb_remove(struct device_d *dev)
{
}

static struct driver_d imx3fb_driver = {
	.name = "imx-ipu-fb",
	.probe = imxfb_probe,
	.remove = imxfb_remove,
};
device_platform_driver(imx3fb_driver);

/**
 * @file
 * @brief Programming the video controller in the i.MX35 CPU
 */
