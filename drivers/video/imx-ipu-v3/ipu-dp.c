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

#include "imx-ipu-v3.h"
#include "ipu-prv.h"

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC

#define DP_COM_CONF		0x0
#define DP_GRAPH_WIND_CTRL	0x0004
#define DP_FG_POS		0x0008
#define DP_CSC_A_0		0x0044
#define DP_CSC_A_1		0x0048
#define DP_CSC_A_2		0x004C
#define DP_CSC_A_3		0x0050
#define DP_CSC_0		0x0054
#define DP_CSC_1		0x0058

#define DP_COM_CONF_FG_EN		(1 << 0)
#define DP_COM_CONF_GWSEL		(1 << 1)
#define DP_COM_CONF_GWAM		(1 << 2)
#define DP_COM_CONF_GWCKE		(1 << 3)
#define DP_COM_CONF_CSC_DEF_MASK	(3 << 8)
#define DP_COM_CONF_CSC_DEF_OFFSET	8
#define DP_COM_CONF_CSC_DEF_FG		(3 << 8)
#define DP_COM_CONF_CSC_DEF_BG		(2 << 8)
#define DP_COM_CONF_CSC_DEF_BOTH	(1 << 8)

#define IPUV3_NUM_FLOWS		3

struct ipu_dp_priv;

struct ipu_dp {
	u32 flow;
	bool in_use;
	bool foreground;
	enum ipu_color_space in_cs;
};

struct ipu_flow {
	struct ipu_dp foreground;
	struct ipu_dp background;
	enum ipu_color_space out_cs;
	void __iomem *base;
	struct ipu_dp_priv *priv;
};

struct ipu_dp_priv {
	struct ipu_soc *ipu;
	struct device_d *dev;
	void __iomem *base;
	struct ipu_flow flow[IPUV3_NUM_FLOWS];
	int use_count;
};

static u32 ipu_dp_flow_base[] = {DP_SYNC, DP_ASYNC0, DP_ASYNC1};

static inline struct ipu_flow *to_flow(struct ipu_dp *dp)
{
	if (dp->foreground)
		return container_of(dp, struct ipu_flow, foreground);
	else
		return container_of(dp, struct ipu_flow, background);
}

int ipu_dp_set_global_alpha(struct ipu_dp *dp, bool enable,
		u8 alpha, bool bg_chan)
{
	struct ipu_flow *flow = to_flow(dp);
	struct ipu_dp_priv *priv = flow->priv;
	u32 reg;

	reg = ipureadl(flow->base + DP_COM_CONF);
	if (bg_chan)
		reg &= ~DP_COM_CONF_GWSEL;
	else
		reg |= DP_COM_CONF_GWSEL;
	ipuwritel("dp", reg, flow->base + DP_COM_CONF);

	if (enable) {
		reg = ipureadl(flow->base + DP_GRAPH_WIND_CTRL) & 0x00FFFFFFL;
		ipuwritel("dp", reg | ((u32) alpha << 24),
			     flow->base + DP_GRAPH_WIND_CTRL);

		reg = ipureadl(flow->base + DP_COM_CONF);
		ipuwritel("dp", reg | DP_COM_CONF_GWAM, flow->base + DP_COM_CONF);
	} else {
		reg = ipureadl(flow->base + DP_COM_CONF);
		ipuwritel("dp", reg & ~DP_COM_CONF_GWAM, flow->base + DP_COM_CONF);
	}

	ipu_srm_dp_sync_update(priv->ipu);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_dp_set_global_alpha);

int ipu_dp_set_window_pos(struct ipu_dp *dp, u16 x_pos, u16 y_pos)
{
	struct ipu_flow *flow = to_flow(dp);
	struct ipu_dp_priv *priv = flow->priv;

	ipuwritel("dp", (x_pos << 16) | y_pos, flow->base + DP_FG_POS);

	ipu_srm_dp_sync_update(priv->ipu);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_dp_set_window_pos);

static void ipu_dp_csc_init(struct ipu_flow *flow,
		enum ipu_color_space in,
		enum ipu_color_space out,
		u32 place)
{
	u32 reg;

	reg = ipureadl(flow->base + DP_COM_CONF);
	reg &= ~DP_COM_CONF_CSC_DEF_MASK;

	if (in == out) {
		ipuwritel("dp", reg, flow->base + DP_COM_CONF);
		return;
	}

	if (in == IPUV3_COLORSPACE_RGB && out == IPUV3_COLORSPACE_YUV) {
		ipuwritel("dp", 0x099 | (0x12d << 16), flow->base + DP_CSC_A_0);
		ipuwritel("dp", 0x03a | (0x3a9 << 16), flow->base + DP_CSC_A_1);
		ipuwritel("dp", 0x356 | (0x100 << 16), flow->base + DP_CSC_A_2);
		ipuwritel("dp", 0x100 | (0x329 << 16), flow->base + DP_CSC_A_3);
		ipuwritel("dp", 0x3d6 | (0x0000 << 16) | (2 << 30),
				flow->base + DP_CSC_0);
		ipuwritel("dp", 0x200 | (2 << 14) | (0x200 << 16) | (2 << 30),
				flow->base + DP_CSC_1);
	} else {
		ipuwritel("dp", 0x095 | (0x000 << 16), flow->base + DP_CSC_A_0);
		ipuwritel("dp", 0x0cc | (0x095 << 16), flow->base + DP_CSC_A_1);
		ipuwritel("dp", 0x3ce | (0x398 << 16), flow->base + DP_CSC_A_2);
		ipuwritel("dp", 0x095 | (0x0ff << 16), flow->base + DP_CSC_A_3);
		ipuwritel("dp", 0x000 | (0x3e42 << 16) | (1 << 30),
				flow->base + DP_CSC_0);
		ipuwritel("dp", 0x10a | (1 << 14) | (0x3dd6 << 16) | (1 << 30),
				flow->base + DP_CSC_1);
	}

	reg |= place;

	ipuwritel("dp", reg, flow->base + DP_COM_CONF);
}

int ipu_dp_setup_channel(struct ipu_dp *dp,
		enum ipu_color_space in,
		enum ipu_color_space out)
{
	struct ipu_flow *flow = to_flow(dp);
	struct ipu_dp_priv *priv = flow->priv;

	dp->in_cs = in;

	if (!dp->foreground)
		flow->out_cs = out;

	if (flow->foreground.in_cs == flow->background.in_cs) {
		/*
		 * foreground and background are of same colorspace, put
		 * colorspace converter after combining unit.
		 */
		ipu_dp_csc_init(flow, flow->foreground.in_cs, flow->out_cs,
				DP_COM_CONF_CSC_DEF_BOTH);
	} else {
		if (flow->foreground.in_cs == flow->out_cs)
			/*
			 * foreground identical to output, apply color
			 * conversion on background
			 */
			ipu_dp_csc_init(flow, flow->background.in_cs,
					flow->out_cs, DP_COM_CONF_CSC_DEF_BG);
		else
			ipu_dp_csc_init(flow, flow->foreground.in_cs,
					flow->out_cs, DP_COM_CONF_CSC_DEF_FG);
	}

	ipu_srm_dp_sync_update(priv->ipu);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_dp_setup_channel);

int ipu_dp_enable_channel(struct ipu_dp *dp)
{
	struct ipu_flow *flow = to_flow(dp);
	struct ipu_dp_priv *priv = flow->priv;

	if (!priv->use_count)
		ipu_module_enable(priv->ipu, IPU_CONF_DP_EN);

	priv->use_count++;

	if (dp->foreground) {
		u32 reg;

		reg = ipureadl(flow->base + DP_COM_CONF);
		reg |= DP_COM_CONF_FG_EN;
		ipuwritel("dp", reg, flow->base + DP_COM_CONF);

		ipu_srm_dp_sync_update(priv->ipu);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_dp_enable_channel);

void ipu_dp_disable_channel(struct ipu_dp *dp)
{
	struct ipu_flow *flow = to_flow(dp);
	struct ipu_dp_priv *priv = flow->priv;

	priv->use_count--;

	if (dp->foreground) {
		u32 reg, csc;

		reg = ipureadl(flow->base + DP_COM_CONF);
		csc = reg & DP_COM_CONF_CSC_DEF_MASK;
		if (csc == DP_COM_CONF_CSC_DEF_FG)
			reg &= ~DP_COM_CONF_CSC_DEF_MASK;

		reg &= ~DP_COM_CONF_FG_EN;
		ipuwritel("dp", reg, flow->base + DP_COM_CONF);

		ipuwritel("dp", 0, flow->base + DP_FG_POS);
		ipu_srm_dp_sync_update(priv->ipu);
	}

	if (!priv->use_count)
		ipu_module_disable(priv->ipu, IPU_CONF_DP_EN);

	if (priv->use_count < 0)
		priv->use_count = 0;
}
EXPORT_SYMBOL_GPL(ipu_dp_disable_channel);

struct ipu_dp *ipu_dp_get(struct ipu_soc *ipu, unsigned int flow)
{
	struct ipu_dp_priv *priv = ipu->dp_priv;
	struct ipu_dp *dp;

	if ((flow >> 1) >= IPUV3_NUM_FLOWS)
		return ERR_PTR(-EINVAL);

	if (flow & 1)
		dp = &priv->flow[flow >> 1].foreground;
	else
		dp = &priv->flow[flow >> 1].background;

	if (dp->in_use)
		return ERR_PTR(-EBUSY);

	dp->in_use = true;

	return dp;
}
EXPORT_SYMBOL_GPL(ipu_dp_get);

void ipu_dp_put(struct ipu_dp *dp)
{
	dp->in_use = false;
}
EXPORT_SYMBOL_GPL(ipu_dp_put);

int ipu_dp_init(struct ipu_soc *ipu, struct device_d *dev, void __iomem *base)
{
	struct ipu_dp_priv *priv;
	int i;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;
	priv->ipu = ipu;

	ipu->dp_priv = priv;

	priv->base = base;

	for (i = 0; i < IPUV3_NUM_FLOWS; i++) {
		priv->flow[i].foreground.foreground = true;
		priv->flow[i].base = priv->base + ipu_dp_flow_base[i];
		priv->flow[i].priv = priv;
	}

	return 0;
}

void ipu_dp_exit(struct ipu_soc *ipu)
{
}
