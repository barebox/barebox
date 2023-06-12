// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <clock.h>
#include <driver.h>
#include <init.h>
#include <linux/mutex.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx53-regs.h>
#include <mach/imx/imx51-regs.h>

#include "imx-ipu-v3.h"
#include "ipu-prv.h"

void ipuwritel(const char *unit, uint32_t value, void __iomem *reg)
{
	pr_debug("w: %s 0x%08lx -> 0x%08x\n", unit, (unsigned long)reg & 0xfff, value);

	writel(value, reg);
}

uint32_t ipureadl(void __iomem *reg)
{
	uint32_t val;

	val = readl(reg);

	pr_debug("r: 0x%p -> 0x%08x\n", reg - 0x02600000, val);

	return val;
}

static inline u32 ipu_cm_read(struct ipu_soc *ipu, unsigned offset)
{
	return ipureadl(ipu->cm_reg + offset);
}

static inline void ipu_cm_write(struct ipu_soc *ipu, u32 value, unsigned offset)
{
	ipuwritel("cm", value, ipu->cm_reg + offset);
}

static inline u32 ipu_idmac_read(struct ipu_soc *ipu, unsigned offset)
{
	return ipureadl(ipu->idmac_reg + offset);
}

static inline void ipu_idmac_write(struct ipu_soc *ipu, u32 value,
		unsigned offset)
{
	ipuwritel("idmac", value, ipu->idmac_reg + offset);
}

void ipu_srm_dp_sync_update(struct ipu_soc *ipu)
{
	u32 val;

	val = ipu_cm_read(ipu, IPU_SRM_PRI2);
	val |= 0x8;
	ipu_cm_write(ipu, val, IPU_SRM_PRI2);
}
EXPORT_SYMBOL_GPL(ipu_srm_dp_sync_update);

struct ipu_ch_param __iomem *ipu_get_cpmem(struct ipuv3_channel *channel)
{
	struct ipu_soc *ipu = channel->ipu;

	return ipu->cpmem_base + channel->num;
}
EXPORT_SYMBOL_GPL(ipu_get_cpmem);

void ipu_cpmem_set_high_priority(struct ipuv3_channel *channel)
{
	struct ipu_soc *ipu = channel->ipu;
	struct ipu_ch_param __iomem *p = ipu_get_cpmem(channel);
	u32 val;

	if (ipu->ipu_type == IPUV3EX)
		ipu_ch_param_write_field(p, IPU_FIELD_ID, 1);

	val = ipu_idmac_read(ipu, IDMAC_CHA_PRI(channel->num));
	val |= 1 << (channel->num % 32);
	ipu_idmac_write(ipu, val, IDMAC_CHA_PRI(channel->num));
};
EXPORT_SYMBOL_GPL(ipu_cpmem_set_high_priority);

void ipu_ch_param_write_field(struct ipu_ch_param __iomem *base, u32 wbs, u32 v)
{
	u32 bit = (wbs >> 8) % 160;
	u32 size = wbs & 0xff;
	u32 word = (wbs >> 8) / 160;
	u32 i = bit / 32;
	u32 ofs = bit % 32;
	u32 mask = (1 << size) - 1;
	u32 val;

	pr_debug("%s %d %d %d 0x%08x\n", __func__, word, bit , size, v);

	val = ipureadl(&base->word[word].data[i]);
	val &= ~(mask << ofs);
	val |= v << ofs;
	ipuwritel("chp", val, &base->word[word].data[i]);

	if ((bit + size - 1) / 32 > i) {
		val = ipureadl(&base->word[word].data[i + 1]);
		val &= ~(mask >> (ofs ? (32 - ofs) : 0));
		val |= v >> (ofs ? (32 - ofs) : 0);
		ipuwritel("chp", val, &base->word[word].data[i + 1]);
	}
}
EXPORT_SYMBOL_GPL(ipu_ch_param_write_field);

u32 ipu_ch_param_read_field(struct ipu_ch_param __iomem *base, u32 wbs)
{
	u32 bit = (wbs >> 8) % 160;
	u32 size = wbs & 0xff;
	u32 word = (wbs >> 8) / 160;
	u32 i = bit / 32;
	u32 ofs = bit % 32;
	u32 mask = (1 << size) - 1;
	u32 val = 0;

	pr_debug("%s %d %d %d\n", __func__, word, bit , size);

	val = (ipureadl(&base->word[word].data[i]) >> ofs) & mask;

	if ((bit + size - 1) / 32 > i) {
		u32 tmp;
		tmp = ipureadl(&base->word[word].data[i + 1]);
		tmp &= mask >> (ofs ? (32 - ofs) : 0);
		val |= tmp << (ofs ? (32 - ofs) : 0);
	}

	return val;
}
EXPORT_SYMBOL_GPL(ipu_ch_param_read_field);

int ipu_cpmem_set_format_rgb(struct ipu_ch_param __iomem *p,
		const struct ipu_rgb *rgb)
{
	int bpp = 0, npb = 0, ro, go, bo, to;

	ro = rgb->bits_per_pixel - rgb->red.length - rgb->red.offset;
	go = rgb->bits_per_pixel - rgb->green.length - rgb->green.offset;
	bo = rgb->bits_per_pixel - rgb->blue.length - rgb->blue.offset;
	to = rgb->bits_per_pixel - rgb->transp.length - rgb->transp.offset;

	ipu_ch_param_write_field(p, IPU_FIELD_WID0, rgb->red.length - 1);
	ipu_ch_param_write_field(p, IPU_FIELD_OFS0, ro);
	ipu_ch_param_write_field(p, IPU_FIELD_WID1, rgb->green.length - 1);
	ipu_ch_param_write_field(p, IPU_FIELD_OFS1, go);
	ipu_ch_param_write_field(p, IPU_FIELD_WID2, rgb->blue.length - 1);
	ipu_ch_param_write_field(p, IPU_FIELD_OFS2, bo);

	if (rgb->transp.length) {
		ipu_ch_param_write_field(p, IPU_FIELD_WID3,
				rgb->transp.length - 1);
		ipu_ch_param_write_field(p, IPU_FIELD_OFS3, to);
	} else {
		ipu_ch_param_write_field(p, IPU_FIELD_WID3, 7);
		ipu_ch_param_write_field(p, IPU_FIELD_OFS3,
				rgb->bits_per_pixel);
	}

	switch (rgb->bits_per_pixel) {
	case 32:
		bpp = 0;
		npb = 15;
		break;
	case 24:
		bpp = 1;
		npb = 19;
		break;
	case 16:
		bpp = 3;
		npb = 31;
		break;
	case 8:
		bpp = 5;
		npb = 63;
		break;
	default:
		return -EINVAL;
	}
	ipu_ch_param_write_field(p, IPU_FIELD_BPP, bpp);
	ipu_ch_param_write_field(p, IPU_FIELD_NPB, npb);
	ipu_ch_param_write_field(p, IPU_FIELD_PFS, 7); /* rgb mode */

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_cpmem_set_format_rgb);

static struct ipu_rgb def_rgb_32 = {
	.red	= { .offset = 16, .length = 8, },
	.green	= { .offset =  8, .length = 8, },
	.blue	= { .offset =  0, .length = 8, },
	.transp = { .offset = 24, .length = 8, },
	.bits_per_pixel = 32,
};

static struct ipu_rgb def_bgr_32 = {
	.red	= { .offset =  0, .length = 8, },
	.green	= { .offset =  8, .length = 8, },
	.blue	= { .offset = 16, .length = 8, },
	.transp = { .offset = 24, .length = 8, },
	.bits_per_pixel = 32,
};

static struct ipu_rgb def_rgb_24 = {
	.red	= { .offset = 16, .length = 8, },
	.green	= { .offset =  8, .length = 8, },
	.blue	= { .offset =  0, .length = 8, },
	.transp = { .offset =  0, .length = 0, },
	.bits_per_pixel = 24,
};

static struct ipu_rgb def_bgr_24 = {
	.red	= { .offset =  0, .length = 8, },
	.green	= { .offset =  8, .length = 8, },
	.blue	= { .offset = 16, .length = 8, },
	.transp = { .offset =  0, .length = 0, },
	.bits_per_pixel = 24,
};

static struct ipu_rgb def_rgb_16 = {
	.red	= { .offset = 11, .length = 5, },
	.green	= { .offset =  5, .length = 6, },
	.blue	= { .offset =  0, .length = 5, },
	.transp = { .offset =  0, .length = 0, },
	.bits_per_pixel = 16,
};

static struct ipu_rgb def_bgr_16 = {
	.red	= { .offset =  0, .length = 5, },
	.green	= { .offset =  5, .length = 6, },
	.blue	= { .offset = 11, .length = 5, },
	.transp = { .offset =  0, .length = 0, },
	.bits_per_pixel = 16,
};

struct ipu_rgb *drm_fourcc_to_rgb(u32 drm_fourcc)
{
	switch (drm_fourcc) {
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
		return &def_bgr_32;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		return &def_rgb_32;
	case DRM_FORMAT_BGR888:
		return &def_bgr_24;
	case DRM_FORMAT_RGB888:
		return &def_rgb_24;
	case DRM_FORMAT_RGB565:
		return &def_rgb_16;
	case DRM_FORMAT_BGR565:
		return &def_bgr_16;
	default:
		return NULL;
	}
}

#define Y_OFFSET(pix, x, y)	((x) + pix->width * (y))
#define U_OFFSET(pix, x, y)	((pix->width * pix->height) + \
					(pix->width * (y) / 4) + (x) / 2)
#define V_OFFSET(pix, x, y)	((pix->width * pix->height) + \
					(pix->width * pix->height / 4) + \
					(pix->width * (y) / 4) + (x) / 2)

int ipu_cpmem_set_fmt(struct ipu_ch_param __iomem *cpmem, u32 drm_fourcc)
{
	switch (drm_fourcc) {
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
		ipu_cpmem_set_format_rgb(cpmem, &def_bgr_32);
		break;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		ipu_cpmem_set_format_rgb(cpmem, &def_rgb_32);
		break;
	case DRM_FORMAT_BGR888:
		ipu_cpmem_set_format_rgb(cpmem, &def_bgr_24);
		break;
	case DRM_FORMAT_RGB888:
		ipu_cpmem_set_format_rgb(cpmem, &def_rgb_24);
		break;
	case DRM_FORMAT_RGB565:
		ipu_cpmem_set_format_rgb(cpmem, &def_rgb_16);
		break;
	case DRM_FORMAT_BGR565:
		ipu_cpmem_set_format_rgb(cpmem, &def_bgr_16);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_cpmem_set_fmt);

struct ipuv3_channel *ipu_idmac_get(struct ipu_soc *ipu, unsigned num)
{
	struct ipuv3_channel *channel;

	dev_dbg(ipu->dev, "%s %d\n", __func__, num);

	if (num > 63)
		return ERR_PTR(-ENODEV);

	mutex_lock(&ipu->channel_lock);

	channel = &ipu->channel[num];

	if (channel->busy) {
		channel = ERR_PTR(-EBUSY);
		goto out;
	}

	channel->busy = true;
	channel->num = num;

out:
	mutex_unlock(&ipu->channel_lock);

	return channel;
}
EXPORT_SYMBOL_GPL(ipu_idmac_get);

void ipu_idmac_put(struct ipuv3_channel *channel)
{
	struct ipu_soc *ipu = channel->ipu;

	dev_dbg(ipu->dev, "%s %d\n", __func__, channel->num);

	mutex_lock(&ipu->channel_lock);

	channel->busy = false;

	mutex_unlock(&ipu->channel_lock);
}
EXPORT_SYMBOL_GPL(ipu_idmac_put);

#define idma_mask(ch)			(1 << (ch & 0x1f))

void ipu_idmac_set_double_buffer(struct ipuv3_channel *channel,
		bool doublebuffer)
{
	struct ipu_soc *ipu = channel->ipu;
	u32 reg;

	reg = ipu_cm_read(ipu, IPU_CHA_DB_MODE_SEL(channel->num));
	if (doublebuffer)
		reg |= idma_mask(channel->num);
	else
		reg &= ~idma_mask(channel->num);
	ipu_cm_write(ipu, reg, IPU_CHA_DB_MODE_SEL(channel->num));
}
EXPORT_SYMBOL_GPL(ipu_idmac_set_double_buffer);

int ipu_module_enable(struct ipu_soc *ipu, u32 mask)
{
	u32 val;

	val = ipu_cm_read(ipu, IPU_DISP_GEN);

	if (mask & IPU_CONF_DI0_EN)
		val |= IPU_DI0_COUNTER_RELEASE;
	if (mask & IPU_CONF_DI1_EN)
		val |= IPU_DI1_COUNTER_RELEASE;

	ipu_cm_write(ipu, val, IPU_DISP_GEN);

	val = ipu_cm_read(ipu, IPU_CONF);
	val |= mask;
	ipu_cm_write(ipu, val, IPU_CONF);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_module_enable);

int ipu_module_disable(struct ipu_soc *ipu, u32 mask)
{
	u32 val;

	val = ipu_cm_read(ipu, IPU_CONF);
	val &= ~mask;
	ipu_cm_write(ipu, val, IPU_CONF);

	val = ipu_cm_read(ipu, IPU_DISP_GEN);

	if (mask & IPU_CONF_DI0_EN)
		val &= ~IPU_DI0_COUNTER_RELEASE;
	if (mask & IPU_CONF_DI1_EN)
		val &= ~IPU_DI1_COUNTER_RELEASE;

	ipu_cm_write(ipu, val, IPU_DISP_GEN);

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_module_disable);

void ipu_idmac_select_buffer(struct ipuv3_channel *channel, u32 buf_num)
{
	struct ipu_soc *ipu = channel->ipu;
	unsigned int chno = channel->num;

	/* Mark buffer as ready. */
	if (buf_num == 0)
		ipu_cm_write(ipu, idma_mask(chno), IPU_CHA_BUF0_RDY(chno));
	else
		ipu_cm_write(ipu, idma_mask(chno), IPU_CHA_BUF1_RDY(chno));
}
EXPORT_SYMBOL_GPL(ipu_idmac_select_buffer);

int ipu_idmac_enable_channel(struct ipuv3_channel *channel)
{
	struct ipu_soc *ipu = channel->ipu;
	u32 val;

	val = ipu_idmac_read(ipu, IDMAC_CHA_EN(channel->num));
	val |= idma_mask(channel->num);
	ipu_idmac_write(ipu, val, IDMAC_CHA_EN(channel->num));

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_idmac_enable_channel);

int ipu_idmac_wait_busy(struct ipuv3_channel *channel, int ms)
{
	struct ipu_soc *ipu = channel->ipu;
	uint64_t start;

	start = get_time_ns();

	while (ipu_idmac_read(ipu, IDMAC_CHA_BUSY(channel->num)) &
			idma_mask(channel->num)) {
		if (is_timeout(start, ms * MSECOND))
			return -ETIMEDOUT;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_idmac_wait_busy);

int ipu_idmac_disable_channel(struct ipuv3_channel *channel)
{
	struct ipu_soc *ipu = channel->ipu;
	u32 val;

	/* Disable DMA channel(s) */
	val = ipu_idmac_read(ipu, IDMAC_CHA_EN(channel->num));
	val &= ~idma_mask(channel->num);
	ipu_idmac_write(ipu, val, IDMAC_CHA_EN(channel->num));

	/* Set channel buffers NOT to be ready */
	ipu_cm_write(ipu, 0xf0000000, IPU_GPR); /* write one to clear */

	if (ipu_cm_read(ipu, IPU_CHA_BUF0_RDY(channel->num)) &
			idma_mask(channel->num)) {
		ipu_cm_write(ipu, idma_mask(channel->num),
			     IPU_CHA_BUF0_RDY(channel->num));
	}

	if (ipu_cm_read(ipu, IPU_CHA_BUF1_RDY(channel->num)) &
			idma_mask(channel->num)) {
		ipu_cm_write(ipu, idma_mask(channel->num),
			     IPU_CHA_BUF1_RDY(channel->num));
	}

	ipu_cm_write(ipu, 0x0, IPU_GPR); /* write one to set */

	/* Reset the double buffer */
	val = ipu_cm_read(ipu, IPU_CHA_DB_MODE_SEL(channel->num));
	val &= ~idma_mask(channel->num);
	ipu_cm_write(ipu, val, IPU_CHA_DB_MODE_SEL(channel->num));

	return 0;
}
EXPORT_SYMBOL_GPL(ipu_idmac_disable_channel);

static int ipu_memory_reset(struct ipu_soc *ipu)
{
	uint64_t start;

	ipu_cm_write(ipu, 0x807FFFFF, IPU_MEM_RST);

	start = get_time_ns();

	while (ipu_cm_read(ipu, IPU_MEM_RST) & 0x80000000) {
		if (is_timeout(start, SECOND))
			return -ETIMEDOUT;
	}

	return 0;
}

static int imx6_ipu_reset(struct ipu_soc *ipu)
{
	uint32_t val;
	int ret;
	void __iomem *reg;

	reg = (void *)MX6_SRC_BASE_ADDR;
	val = ipureadl(reg);
	if (ipu->base == (void *)MX6_IPU1_BASE_ADDR)
		val |= (1 << 3);
	else
		val |= (1 << 12);

	ipuwritel("reset", val, reg);

	ret = wait_on_timeout(100 * MSECOND, !(readl(reg) & (1 << 3)));

	return ret;

}

static int imx5_ipu_reset(void __iomem *src_base)
{
	uint32_t val;
	int ret;

	val = ipureadl(src_base);
	val |= (1 << 3);
	ipuwritel("reset", val, src_base);

	ret = wait_on_timeout(100 * MSECOND, !(readl(src_base) & (1 << 3)));

	return ret;

}

static int imx51_ipu_reset(struct ipu_soc *ipu)
{
	return imx5_ipu_reset((void *)MX51_SRC_BASE_ADDR);
}

static int imx53_ipu_reset(struct ipu_soc *ipu)
{
	return imx5_ipu_reset((void *)MX53_SRC_BASE_ADDR);
}

struct ipu_devtype {
	const char *name;
	unsigned long cm_ofs;
	unsigned long cpmem_ofs;
	unsigned long srm_ofs;
	unsigned long tpm_ofs;
	unsigned long disp0_ofs;
	unsigned long disp1_ofs;
	unsigned long dc_tmpl_ofs;
	unsigned long vdi_ofs;
	enum ipuv3_type type;
	int (*reset)(struct ipu_soc *ipu);
};

static struct ipu_devtype ipu_type_imx51 = {
	.name = "IPUv3EX",
	.cm_ofs = 0x1e000000,
	.cpmem_ofs = 0x1f000000,
	.srm_ofs = 0x1f040000,
	.tpm_ofs = 0x1f060000,
	.disp0_ofs = 0x1e040000,
	.disp1_ofs = 0x1e048000,
	.dc_tmpl_ofs = 0x1f080000,
	.vdi_ofs = 0x1e068000,
	.type = IPUV3EX,
	.reset = imx51_ipu_reset,
};

static struct ipu_devtype ipu_type_imx53 = {
	.name = "IPUv3M",
	.cm_ofs = 0x06000000,
	.cpmem_ofs = 0x07000000,
	.srm_ofs = 0x07040000,
	.tpm_ofs = 0x07060000,
	.disp0_ofs = 0x06040000,
	.disp1_ofs = 0x06048000,
	.dc_tmpl_ofs = 0x07080000,
	.vdi_ofs = 0x06068000,
	.type = IPUV3M,
	.reset = imx53_ipu_reset,
};

static struct ipu_devtype ipu_type_imx6q = {
	.name = "IPUv3H",
	.cm_ofs = 0x00200000,
	.cpmem_ofs = 0x00300000,
	.srm_ofs = 0x00340000,
	.tpm_ofs = 0x00360000,
	.disp0_ofs = 0x00240000,
	.disp1_ofs = 0x00248000,
	.dc_tmpl_ofs = 0x00380000,
	.vdi_ofs = 0x00268000,
	.type = IPUV3H,
	.reset = imx6_ipu_reset,
};

static struct of_device_id imx_ipu_dt_ids[] = {
	{ .compatible = "fsl,imx51-ipu", .data = &ipu_type_imx51, },
	{ .compatible = "fsl,imx53-ipu", .data = &ipu_type_imx53, },
	{ .compatible = "fsl,imx6q-ipu", .data = &ipu_type_imx6q, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_ipu_dt_ids);

static int ipu_submodules_init(struct ipu_soc *ipu,
		struct device *dev, void __iomem *ipu_base,
		struct clk *ipu_clk)
{
	char *unit;
	int ret;
	const struct ipu_devtype *devtype = ipu->devtype;

	ret = ipu_di_init(ipu, dev, 0, ipu_base + devtype->disp0_ofs,
			IPU_CONF_DI0_EN, ipu_clk);
	if (ret) {
		unit = "di0";
		goto err_di_0;
	}

	ret = ipu_di_init(ipu, dev, 1, ipu_base + devtype->disp1_ofs,
			IPU_CONF_DI1_EN, ipu_clk);
	if (ret) {
		unit = "di1";
		goto err_di_1;
	}

	ret = ipu_dc_init(ipu, dev, ipu_base + devtype->cm_ofs +
			IPU_CM_DC_REG_OFS, ipu_base + devtype->dc_tmpl_ofs);
	if (ret) {
		unit = "dc_template";
		goto err_dc;
	}

	ret = ipu_dmfc_init(ipu, dev, ipu_base +
			devtype->cm_ofs + IPU_CM_DMFC_REG_OFS, ipu_clk);
	if (ret) {
		unit = "dmfc";
		goto err_dmfc;
	}

	ret = ipu_dp_init(ipu, dev, ipu_base + devtype->srm_ofs);
	if (ret) {
		unit = "dp";
		goto err_dp;
	}

	return 0;

err_dp:
	ipu_dmfc_exit(ipu);
err_dmfc:
	ipu_dc_exit(ipu);
err_dc:
	ipu_di_exit(ipu, 1);
err_di_1:
	ipu_di_exit(ipu, 0);
err_di_0:
	dev_err(dev, "init %s failed with %d\n", unit, ret);
	return ret;
}

static void ipu_submodules_exit(struct ipu_soc *ipu)
{
	ipu_dp_exit(ipu);
	ipu_dmfc_exit(ipu);
	ipu_dc_exit(ipu);
	ipu_di_exit(ipu, 1);
	ipu_di_exit(ipu, 0);
}

struct ipu_platform_reg {
	struct ipu_client_platformdata pdata;
	const char *name;
};

static struct ipu_platform_reg client_reg[] = {
	{
		.pdata = {
			.di = 0,
			.dc = 5,
			.dp = IPU_DP_FLOW_SYNC_BG,
			.dma[0] = IPUV3_CHANNEL_MEM_BG_SYNC,
			.dma[1] = IPUV3_CHANNEL_MEM_FG_SYNC,
		},
		.name = "imx-ipuv3-crtc",
	}, {
		.pdata = {
			.di = 1,
			.dc = 1,
			.dp = -EINVAL,
			.dma[0] = IPUV3_CHANNEL_MEM_DC_SYNC,
			.dma[1] = -EINVAL,
		},
		.name = "imx-ipuv3-crtc",
        },
};

static int ipu_client_id;

static int ipu_add_subdevice_pdata(struct device *ipu_dev,
				   struct ipu_platform_reg *reg)
{
	struct device *dev;
	int ret;

	dev = device_alloc(reg->name, ipu_client_id++);
	dev->parent = ipu_dev;
	device_add_data(dev, &reg->pdata, sizeof(reg->pdata));
	((struct ipu_client_platformdata *)dev->platform_data)->device_node = ipu_dev->of_node;

	ret = platform_device_register(dev);

	return ret;
}

static int ipu_add_client_devices(struct ipu_soc *ipu)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(client_reg); i++) {
		struct ipu_platform_reg *reg = &client_reg[i];
		ret = ipu_add_subdevice_pdata(ipu->dev, reg);
		if (ret)
			goto err_register;
	}

	return 0;

err_register:

	return ret;
}

static int ipu_probe(struct device *dev)
{
	struct resource *iores;
	struct ipu_soc *ipu;
	void __iomem *ipu_base;
	int i, ret;
	const struct ipu_devtype *devtype;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ipu_base = IOMEM(iores->start);

	ipu = xzalloc(sizeof(*ipu));

	ipu->base = ipu_base;

	for (i = 0; i < 64; i++)
		ipu->channel[i].ipu = ipu;
	ipu->devtype = devtype;
	ipu->ipu_type = devtype->type;

	dev_dbg(dev, "cm_reg:   0x%p\n",
			ipu_base + devtype->cm_ofs);
	dev_dbg(dev, "idmac:    0x%p\n",
			ipu_base + devtype->cm_ofs + IPU_CM_IDMAC_REG_OFS);
	dev_dbg(dev, "cpmem:    0x%p\n",
			ipu_base + devtype->cpmem_ofs);
	dev_dbg(dev, "disp0:    0x%p\n",
			ipu_base + devtype->disp0_ofs);
	dev_dbg(dev, "disp1:    0x%p\n",
			ipu_base + devtype->disp1_ofs);
	dev_dbg(dev, "srm:      0x%p\n",
			ipu_base + devtype->srm_ofs);
	dev_dbg(dev, "tpm:      0x%p\n",
			ipu_base + devtype->tpm_ofs);
	dev_dbg(dev, "dc:       0x%p\n",
			ipu_base + devtype->cm_ofs + IPU_CM_DC_REG_OFS);
	dev_dbg(dev, "ic:       0x%p\n",
			ipu_base + devtype->cm_ofs + IPU_CM_IC_REG_OFS);
	dev_dbg(dev, "dmfc:     0x%p\n",
			ipu_base + devtype->cm_ofs + IPU_CM_DMFC_REG_OFS);
	dev_dbg(dev, "vdi:      0x%p\n",
			ipu_base + devtype->vdi_ofs);

	ipu->cm_reg = ipu_base + devtype->cm_ofs;
	ipu->idmac_reg = ipu_base + devtype->cm_ofs + IPU_CM_IDMAC_REG_OFS;
	ipu->cpmem_base = ipu_base + devtype->cpmem_ofs;

	ipu->clk = clk_get(dev, "bus");
	if (IS_ERR(ipu->clk)) {
		dev_err(dev, "clk_get failed: %pe\n", ipu->clk);
		return PTR_ERR(ipu->clk);
	}

	dev->priv = ipu;

	ret = clk_enable(ipu->clk);
	if (ret)
		return ret;

	ipu->dev = dev;

	ret = devtype->reset(ipu);
	if (ret) {
		dev_err(dev, "failed to reset: %s\n", strerror(-ret));
		goto out_failed_reset;
	}

	ret = ipu_memory_reset(ipu);
	if (ret)
		goto out_failed_reset;

	/* Set MCU_T to divide MCU access window into 2 */
	ipu_cm_write(ipu, 0x00400000L | (IPU_MCU_T_DEFAULT << 18),
			IPU_DISP_GEN);

	ret = ipu_submodules_init(ipu, dev, ipu_base, ipu->clk);
	if (ret)
		goto failed_submodules_init;

	ret = ipu_add_client_devices(ipu);
	if (ret) {
		dev_err(dev, "adding client devices failed with %d\n",
				ret);
		goto failed_add_clients;
	}

	dev_info(dev, "%s probed\n", devtype->name);

	return 0;

failed_add_clients:
	ipu_submodules_exit(ipu);
failed_submodules_init:
out_failed_reset:
	clk_disable(ipu->clk);
	return ret;
}

static struct driver imx_ipu_driver = {
	.name = "imx-ipuv3",
	.of_compatible = imx_ipu_dt_ids,
	.probe = ipu_probe,
};

device_platform_driver(imx_ipu_driver);

MODULE_DESCRIPTION("i.MX IPU v3 driver");
MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
MODULE_LICENSE("GPL");
