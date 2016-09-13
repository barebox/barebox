/*
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <common.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <malloc.h>
#include <video/media-bus-format.h>

#include "imx-ipu-v3.h"
#include "ipu-prv.h"

#define DC_MAP_CONF_PTR(n)	(0x108 + ((n) & ~0x1) * 2)
#define DC_MAP_CONF_VAL(n)	(0x144 + ((n) & ~0x1) * 2)

#define DC_EVT_NF		0
#define DC_EVT_NL		1
#define DC_EVT_EOF		2
#define DC_EVT_NFIELD		3
#define DC_EVT_EOL		4
#define DC_EVT_EOFIELD		5
#define DC_EVT_NEW_ADDR		6
#define DC_EVT_NEW_CHAN		7
#define DC_EVT_NEW_DATA		8

#define DC_EVT_NEW_ADDR_W_0	0
#define DC_EVT_NEW_ADDR_W_1	1
#define DC_EVT_NEW_CHAN_W_0	2
#define DC_EVT_NEW_CHAN_W_1	3
#define DC_EVT_NEW_DATA_W_0	4
#define DC_EVT_NEW_DATA_W_1	5
#define DC_EVT_NEW_ADDR_R_0	6
#define DC_EVT_NEW_ADDR_R_1	7
#define DC_EVT_NEW_CHAN_R_0	8
#define DC_EVT_NEW_CHAN_R_1	9
#define DC_EVT_NEW_DATA_R_0	10
#define DC_EVT_NEW_DATA_R_1	11

#define DC_WR_CH_CONF		0x0
#define DC_WR_CH_ADDR		0x4
#define DC_RL_CH(evt)		(8 + ((evt) & ~0x1) * 2)

#define DC_GEN			0xd4
#define DC_DISP_CONF1(disp)	(0xd8 + (disp) * 4)
#define DC_DISP_CONF2(disp)	(0xe8 + (disp) * 4)
#define DC_STAT			0x1c8

#define WROD(lf)		(0x18 | ((lf) << 1))
#define WRG			0x01
#define WCLK			0xc9

#define SYNC_WAVE 0
#define NULL_WAVE (-1)

#define DC_GEN_SYNC_1_6_SYNC	(2 << 1)
#define DC_GEN_SYNC_PRIORITY_1	(1 << 7)

#define DC_WR_CH_CONF_WORD_SIZE_8		(0 << 0)
#define DC_WR_CH_CONF_WORD_SIZE_16		(1 << 0)
#define DC_WR_CH_CONF_WORD_SIZE_24		(2 << 0)
#define DC_WR_CH_CONF_WORD_SIZE_32		(3 << 0)
#define DC_WR_CH_CONF_DISP_ID_PARALLEL(i)	(((i) & 0x1) << 3)
#define DC_WR_CH_CONF_DISP_ID_SERIAL		(2 << 3)
#define DC_WR_CH_CONF_DISP_ID_ASYNC		(3 << 4)
#define DC_WR_CH_CONF_FIELD_MODE		(1 << 9)
#define DC_WR_CH_CONF_PROG_TYPE_NORMAL		(4 << 5)
#define DC_WR_CH_CONF_PROG_TYPE_MASK		(7 << 5)
#define DC_WR_CH_CONF_PROG_DI_ID		(1 << 2)
#define DC_WR_CH_CONF_PROG_DISP_ID(i)		(((i) & 0x1) << 3)

#define IPU_DC_NUM_CHANNELS	10

struct ipu_dc_priv;

enum ipu_dc_map {
	IPU_DC_MAP_RGB24,
	IPU_DC_MAP_RGB565,
	IPU_DC_MAP_GBR24, /* TVEv2 */
	IPU_DC_MAP_BGR666,
	IPU_DC_MAP_BGR24,
};

struct ipu_dc {
	/* The display interface number assigned to this dc channel */
	unsigned int		di;
	void __iomem		*base;
	struct ipu_dc_priv	*priv;
	int			chno;
	bool			in_use;
};

struct ipu_dc_priv {
	void __iomem		*dc_reg;
	void __iomem		*dc_tmpl_reg;
	struct ipu_soc		*ipu;
	struct device_d		*dev;
	struct ipu_dc		channels[IPU_DC_NUM_CHANNELS];
};

static void dc_link_event(struct ipu_dc *dc, int event, int addr, int priority)
{
	u32 reg;

	reg = ipureadl(dc->base + DC_RL_CH(event));
	reg &= ~(0xffff << (16 * (event & 0x1)));
	reg |= ((addr << 8) | priority) << (16 * (event & 0x1));
	ipuwritel("dc", reg, dc->base + DC_RL_CH(event));
}

static void dc_write_tmpl(struct ipu_dc *dc, int word, u32 opcode, u32 operand,
		int map, int wave, int glue, int sync, int stop)
{
	struct ipu_dc_priv *priv = dc->priv;
	u32 reg1, reg2;

	if (opcode == WCLK) {
		reg1 = (operand << 20) & 0xfff00000;
		reg2 = operand >> 12 | opcode << 1 | stop << 9;
	} else if (opcode == WRG) {
		reg1 = sync | glue << 4 | ++wave << 11 | ((operand << 15) & 0xffff8000);
		reg2 = operand >> 17 | opcode << 7 | stop << 9;
	} else {
		reg1 = sync | glue << 4 | ++wave << 11 | ++map << 15 | ((operand << 20) & 0xfff00000);
		reg2 = operand >> 12 | opcode << 4 | stop << 9;
	}
	ipuwritel("dc", reg1, priv->dc_tmpl_reg + word * 8);
	ipuwritel("dc", reg2, priv->dc_tmpl_reg + word * 8 + 4);
}

static int ipu_bus_format_to_map(u32 bus_format)
{
	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB888_1X24:
		return IPU_DC_MAP_RGB24;
	case MEDIA_BUS_FMT_RGB565_1X16:
		return IPU_DC_MAP_RGB565;
	case MEDIA_BUS_FMT_GBR888_1X24:
		return IPU_DC_MAP_GBR24;
	case MEDIA_BUS_FMT_RGB666_1X18:
		return IPU_DC_MAP_BGR666;
	case MEDIA_BUS_FMT_BGR888_1X24:
		return IPU_DC_MAP_BGR24;
	default:
		return -EINVAL;
	}
}

int ipu_dc_init_sync(struct ipu_dc *dc, struct ipu_di *di, bool interlaced,
		u32 bus_format, u32 width)
{
	struct ipu_dc_priv *priv = dc->priv;
	u32 reg = 0;
	int map;

	dc->di = ipu_di_get_num(di);

	map = ipu_bus_format_to_map(bus_format);
	if (map < 0) {
		dev_dbg(priv->dev, "IPU_DISP: No MAP\n");
		return map;
	}

	if (interlaced) {
		dc_link_event(dc, DC_EVT_NL, 0, 3);
		dc_link_event(dc, DC_EVT_EOL, 0, 2);
		dc_link_event(dc, DC_EVT_NEW_DATA, 0, 1);

		/* Init template microcode */
		dc_write_tmpl(dc, 0, WROD(0), 0, map, SYNC_WAVE, 0, 8, 1);
	} else {
		if (dc->di) {
			dc_link_event(dc, DC_EVT_NL, 2, 3);
			dc_link_event(dc, DC_EVT_EOL, 3, 2);
			dc_link_event(dc, DC_EVT_NEW_DATA, 1, 1);
			/* Init template microcode */
			dc_write_tmpl(dc, 2, WROD(0), 0, map, SYNC_WAVE, 8, 5, 1);
			dc_write_tmpl(dc, 3, WROD(0), 0, map, SYNC_WAVE, 4, 5, 0);
			dc_write_tmpl(dc, 4, WRG, 0, map, NULL_WAVE, 0, 0, 1);
			dc_write_tmpl(dc, 1, WROD(0), 0, map, SYNC_WAVE, 0, 5, 1);
		} else {
			dc_link_event(dc, DC_EVT_NL, 5, 3);
			dc_link_event(dc, DC_EVT_EOL, 6, 2);
			dc_link_event(dc, DC_EVT_NEW_DATA, 8, 1);
			/* Init template microcode */
			dc_write_tmpl(dc, 5, WROD(0), 0, map, SYNC_WAVE, 8, 5, 1);
			dc_write_tmpl(dc, 6, WROD(0), 0, map, SYNC_WAVE, 4, 5, 0);
			dc_write_tmpl(dc, 7, WRG, 0, map, NULL_WAVE, 0, 0, 1);
			dc_write_tmpl(dc, 8, WROD(0), 0, map, SYNC_WAVE, 0, 5, 1);
		}
	}
	dc_link_event(dc, DC_EVT_NF, 0, 0);
	dc_link_event(dc, DC_EVT_NFIELD, 0, 0);
	dc_link_event(dc, DC_EVT_EOF, 0, 0);
	dc_link_event(dc, DC_EVT_EOFIELD, 0, 0);
	dc_link_event(dc, DC_EVT_NEW_CHAN, 0, 0);
	dc_link_event(dc, DC_EVT_NEW_ADDR, 0, 0);

	reg = ipureadl(dc->base + DC_WR_CH_CONF);
	if (interlaced)
		reg |= DC_WR_CH_CONF_FIELD_MODE;
	else
		reg &= ~DC_WR_CH_CONF_FIELD_MODE;
	ipuwritel("dc", reg, dc->base + DC_WR_CH_CONF);

	ipuwritel("dc", 0x0, dc->base + DC_WR_CH_ADDR);
	ipuwritel("dc", width, priv->dc_reg + DC_DISP_CONF2(dc->di));

	ipu_module_enable(priv->ipu, IPU_CONF_DC_EN);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_dc_init_sync);

void ipu_dc_enable_channel(struct ipu_dc *dc)
{
	int di;
	u32 reg;

	di = dc->di;

	reg = ipureadl(dc->base + DC_WR_CH_CONF);
	reg |= DC_WR_CH_CONF_PROG_TYPE_NORMAL;
	ipuwritel("dc", reg, dc->base + DC_WR_CH_CONF);
}
EXPORT_SYMBOL_GPL(ipu_dc_enable_channel);

void ipu_dc_disable_channel(struct ipu_dc *dc)
{
	struct ipu_dc_priv *priv = dc->priv;
	u32 val;
	int irq = 0, timeout = 50;

	if (dc->chno == 1)
		irq = IPU_IRQ_DC_FC_1;
	else if (dc->chno == 5)
		irq = IPU_IRQ_DP_SF_END;
	else
		return;

	/* should wait for the interrupt here */
	mdelay(50);

	if (dc->di == 0)
		val = 0x00000002;
	else
		val = 0x00000020;

	/* Wait for DC triple buffer to empty */
	while ((ipureadl(priv->dc_reg + DC_STAT) & val) != val) {
		mdelay(2);
		timeout -= 2;
		if (timeout <= 0)
			break;
	}

	val = ipureadl(dc->base + DC_WR_CH_CONF);
	val &= ~DC_WR_CH_CONF_PROG_TYPE_MASK;
	ipuwritel("dc", val, dc->base + DC_WR_CH_CONF);
}
EXPORT_SYMBOL_GPL(ipu_dc_disable_channel);

static void ipu_dc_map_config(struct ipu_dc_priv *priv, enum ipu_dc_map map,
		int byte_num, int offset, int mask)
{
	int ptr = map * 3 + byte_num;
	u32 reg;

	reg = ipureadl(priv->dc_reg + DC_MAP_CONF_VAL(ptr));
	reg &= ~(0xffff << (16 * (ptr & 0x1)));
	reg |= ((offset << 8) | mask) << (16 * (ptr & 0x1));
	ipuwritel("dc", reg, priv->dc_reg + DC_MAP_CONF_VAL(ptr));

	reg = ipureadl(priv->dc_reg + DC_MAP_CONF_PTR(map));
	reg &= ~(0x1f << ((16 * (map & 0x1)) + (5 * byte_num)));
	reg |= ptr << ((16 * (map & 0x1)) + (5 * byte_num));
	ipuwritel("dc", reg, priv->dc_reg + DC_MAP_CONF_PTR(map));
}

static void ipu_dc_map_clear(struct ipu_dc_priv *priv, int map)
{
	u32 reg = ipureadl(priv->dc_reg + DC_MAP_CONF_PTR(map));

	ipuwritel("dc", reg & ~(0xffff << (16 * (map & 0x1))),
		     priv->dc_reg + DC_MAP_CONF_PTR(map));
}

struct ipu_dc *ipu_dc_get(struct ipu_soc *ipu, int channel)
{
	struct ipu_dc_priv *priv = ipu->dc_priv;
	struct ipu_dc *dc;

	if (channel >= IPU_DC_NUM_CHANNELS)
		return ERR_PTR(-ENODEV);

	dc = &priv->channels[channel];

	if (dc->in_use)
		return ERR_PTR(-EBUSY);

	dc->in_use = true;

	return dc;
}
EXPORT_SYMBOL_GPL(ipu_dc_get);

void ipu_dc_put(struct ipu_dc *dc)
{
	dc->in_use = false;
}
EXPORT_SYMBOL_GPL(ipu_dc_put);

int ipu_dc_init(struct ipu_soc *ipu, struct device_d *dev,
		void __iomem *base, void __iomem *template_base)
{
	struct ipu_dc_priv *priv;
	static int channel_offsets[] = { 0, 0x1c, 0x38, 0x54, 0x58, 0x5c,
		0x78, 0, 0x94, 0xb4};
	int i;

	priv = xzalloc(sizeof(*priv));

	priv->dev = dev;
	priv->ipu = ipu;
	priv->dc_reg = base;
	priv->dc_tmpl_reg = template_base;

	for (i = 0; i < IPU_DC_NUM_CHANNELS; i++) {
		priv->channels[i].chno = i;
		priv->channels[i].priv = priv;
		priv->channels[i].base = priv->dc_reg + channel_offsets[i];
	}

	ipuwritel("dc", DC_WR_CH_CONF_WORD_SIZE_24 | DC_WR_CH_CONF_DISP_ID_PARALLEL(1) |
			DC_WR_CH_CONF_PROG_DI_ID,
			priv->channels[1].base + DC_WR_CH_CONF);
	ipuwritel("dc", DC_WR_CH_CONF_WORD_SIZE_24 | DC_WR_CH_CONF_DISP_ID_PARALLEL(0),
			priv->channels[5].base + DC_WR_CH_CONF);

	ipuwritel("dc", DC_GEN_SYNC_1_6_SYNC | DC_GEN_SYNC_PRIORITY_1, priv->dc_reg + DC_GEN);

	ipu->dc_priv = priv;

	dev_dbg(dev, "DC base: 0x%p template base: 0x%p\n",
			base, template_base);

	/* rgb24 */
	ipu_dc_map_clear(priv, IPU_DC_MAP_RGB24);
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB24, 0, 7, 0xff); /* blue */
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB24, 1, 15, 0xff); /* green */
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB24, 2, 23, 0xff); /* red */

	/* rgb565 */
	ipu_dc_map_clear(priv, IPU_DC_MAP_RGB565);
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB565, 0, 4, 0xf8); /* blue */
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB565, 1, 10, 0xfc); /* green */
	ipu_dc_map_config(priv, IPU_DC_MAP_RGB565, 2, 15, 0xf8); /* red */

	/* gbr24 */
	ipu_dc_map_clear(priv, IPU_DC_MAP_GBR24);
	ipu_dc_map_config(priv, IPU_DC_MAP_GBR24, 2, 15, 0xff); /* green */
	ipu_dc_map_config(priv, IPU_DC_MAP_GBR24, 1, 7, 0xff); /* blue */
	ipu_dc_map_config(priv, IPU_DC_MAP_GBR24, 0, 23, 0xff); /* red */

	/* bgr666 */
	ipu_dc_map_clear(priv, IPU_DC_MAP_BGR666);
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR666, 0, 5, 0xfc); /* blue */
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR666, 1, 11, 0xfc); /* green */
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR666, 2, 17, 0xfc); /* red */

	/* bgr24 */
	ipu_dc_map_clear(priv, IPU_DC_MAP_BGR24);
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR24, 2, 7, 0xff); /* red */
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR24, 1, 15, 0xff); /* green */
	ipu_dc_map_config(priv, IPU_DC_MAP_BGR24, 0, 23, 0xff); /* blue */

	return 0;
}

void ipu_dc_exit(struct ipu_soc *ipu)
{
}
