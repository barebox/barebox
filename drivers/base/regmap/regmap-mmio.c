// SPDX-License-Identifier: GPL-2.0
//
// Register map access API - MMIO support
//
// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.

#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>
#include <regmap.h>

#include "internal.h"

struct regmap_mmio_context {
	void __iomem *regs;
	unsigned val_bytes;

	struct clk *clk;

	void (*reg_write)(struct regmap_mmio_context *ctx,
			  unsigned int reg, unsigned int val);
	unsigned int (*reg_read)(struct regmap_mmio_context *ctx,
			         unsigned int reg);
};

static int regmap_mmio_regbits_check(size_t reg_bits)
{
	switch (reg_bits) {
	case 8:
	case 16:
	case 32:
#ifdef CONFIG_64BIT
	case 64:
#endif
		return 0;
	default:
		return -EINVAL;
	}
}

static int regmap_mmio_get_min_stride(size_t val_bits)
{
	int min_stride;

	switch (val_bits) {
	case 8:
		/* The core treats 0 as 1 */
		min_stride = 0;
		return 0;
	case 16:
		min_stride = 2;
		break;
	case 32:
		min_stride = 4;
		break;
#ifdef CONFIG_64BIT
	case 64:
		min_stride = 8;
		break;
#endif
	default:
		return -EINVAL;
	}

	return min_stride;
}

static void regmap_mmio_write8(struct regmap_mmio_context *ctx,
				unsigned int reg,
				unsigned int val)
{
	writeb(val, ctx->regs + reg);
}

static void regmap_mmio_write16le(struct regmap_mmio_context *ctx,
				  unsigned int reg,
				  unsigned int val)
{
	writew(val, ctx->regs + reg);
}

static void regmap_mmio_write32le(struct regmap_mmio_context *ctx,
				  unsigned int reg,
				  unsigned int val)
{
	writel(val, ctx->regs + reg);
}

static void regmap_mmio_write16be(struct regmap_mmio_context *ctx,
				  unsigned int reg,
				  unsigned int val)
{
	iowrite16be(val, ctx->regs + reg);
}

static void regmap_mmio_write32be(struct regmap_mmio_context *ctx,
				  unsigned int reg,
				  unsigned int val)
{
	iowrite32be(val, ctx->regs + reg);
}

#ifdef CONFIG_64BIT
static void regmap_mmio_write64le(struct regmap_mmio_context *ctx,
				  unsigned int reg,
				  unsigned int val)
{
	writeq(val, ctx->regs + reg);
}
#endif

static int regmap_mmio_write(void *context, unsigned int reg, unsigned int val)
{
	struct regmap_mmio_context *ctx = context;
	int ret;

	ret = clk_enable(ctx->clk);
	if (ret < 0)
		return ret;

	ctx->reg_write(ctx, reg, val);

	clk_disable(ctx->clk);

	return 0;
}

static unsigned int regmap_mmio_read8(struct regmap_mmio_context *ctx,
				      unsigned int reg)
{
	return readb(ctx->regs + reg);
}

static unsigned int regmap_mmio_read16le(struct regmap_mmio_context *ctx,
				         unsigned int reg)
{
	return readw(ctx->regs + reg);
}

static unsigned int regmap_mmio_read32le(struct regmap_mmio_context *ctx,
				         unsigned int reg)
{
	return readl(ctx->regs + reg);
}

#ifdef CONFIG_64BIT
static unsigned int regmap_mmio_read64le(struct regmap_mmio_context *ctx,
				         unsigned int reg)
{
	return readq(ctx->regs + reg);
}
#endif

static unsigned int regmap_mmio_read16be(struct regmap_mmio_context *ctx,
				         unsigned int reg)
{
	return ioread16be(ctx->regs + reg);
}

static unsigned int regmap_mmio_read32be(struct regmap_mmio_context *ctx,
				         unsigned int reg)
{
	return ioread32be(ctx->regs + reg);
}

static int regmap_mmio_read(void *context, unsigned int reg, unsigned int *val)
{
	struct regmap_mmio_context *ctx = context;
	int ret;

	ret = clk_enable(ctx->clk);
	if (ret < 0)
		return ret;

	*val = ctx->reg_read(ctx, reg);

	clk_disable(ctx->clk);

	return 0;
}

static const struct regmap_bus regmap_mmio = {
	.reg_write = regmap_mmio_write,
	.reg_read = regmap_mmio_read,
	.val_format_endian_default = REGMAP_ENDIAN_LITTLE,
};

static struct regmap_mmio_context *regmap_mmio_gen_context(struct device_d *dev,
					void __iomem *regs,
					const struct regmap_config *config)
{
	struct regmap_mmio_context *ctx;
	int min_stride;
	int ret;

	ret = regmap_mmio_regbits_check(config->reg_bits);
	if (ret)
		return ERR_PTR(ret);

	if (config->pad_bits)
		return ERR_PTR(-EINVAL);

	min_stride = regmap_mmio_get_min_stride(config->val_bits);
	if (min_stride < 0)
		return ERR_PTR(min_stride);

	if (config->reg_stride < min_stride)
		return ERR_PTR(-EINVAL);

	ctx = xzalloc(sizeof(*ctx));

	ctx->regs = regs;
	ctx->val_bytes = config->val_bits / 8;

	switch (regmap_get_val_endian(dev, &regmap_mmio, config)) {
	case REGMAP_ENDIAN_DEFAULT:
	case REGMAP_ENDIAN_LITTLE:
#ifdef __LITTLE_ENDIAN
	case REGMAP_ENDIAN_NATIVE:
#endif
		switch (config->val_bits) {
		case 8:
			ctx->reg_read = regmap_mmio_read8;
			ctx->reg_write = regmap_mmio_write8;
			break;
		case 16:
			ctx->reg_read = regmap_mmio_read16le;
			ctx->reg_write = regmap_mmio_write16le;
			break;
		case 32:
			ctx->reg_read = regmap_mmio_read32le;
			ctx->reg_write = regmap_mmio_write32le;
			break;
#ifdef CONFIG_64BIT
		case 64:
			ctx->reg_read = regmap_mmio_read64le;
			ctx->reg_write = regmap_mmio_write64le;
			break;
#endif
		default:
			ret = -EINVAL;
			goto err_free;
		}
		break;
	case REGMAP_ENDIAN_BIG:
#ifdef __BIG_ENDIAN
	case REGMAP_ENDIAN_NATIVE:
#endif
		switch (config->val_bits) {
		case 8:
			ctx->reg_read = regmap_mmio_read8;
			ctx->reg_write = regmap_mmio_write8;
			break;
		case 16:
			ctx->reg_read = regmap_mmio_read16be;
			ctx->reg_write = regmap_mmio_write16be;
			break;
		case 32:
			ctx->reg_read = regmap_mmio_read32be;
			ctx->reg_write = regmap_mmio_write32be;
			break;
		default:
			ret = -EINVAL;
			goto err_free;
		}
		break;
	default:
		ret = -EINVAL;
		goto err_free;
	}

	return ctx;

err_free:
	kfree(ctx);

	return ERR_PTR(ret);
}

struct regmap *regmap_init_mmio_clk(struct device_d *dev,
				    const char *clk_id,
				    void __iomem *regs,
				    const struct regmap_config *config)
{
	struct regmap_mmio_context *ctx;

	ctx = regmap_mmio_gen_context(dev, regs, config);
	if (IS_ERR(ctx))
		return ERR_CAST(ctx);

	if (clk_id) {
		ctx->clk = clk_get(dev, clk_id);
		if (IS_ERR(ctx->clk)) {
			kfree(ctx);
			return ERR_CAST(ctx->clk);
		}
	}

	return regmap_init(dev, &regmap_mmio, ctx, config);
}

int regmap_mmio_attach_clk(struct regmap *map, struct clk *clk)
{
	struct regmap_mmio_context *ctx = map->bus_context;

	ctx->clk = clk;

	return 0;
}

void regmap_mmio_detach_clk(struct regmap *map)
{
	struct regmap_mmio_context *ctx = map->bus_context;

	ctx->clk = NULL;
}
