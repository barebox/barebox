// SPDX-License-Identifier: GPL-2.0-only
/*
 * omap-rng.c - RNG driver for TI OMAP CPU family
 *
 * Author: Deepak Saxena <dsaxena@plexity.net>
 *
 * Copyright 2005 (c) MontaVista Software, Inc.
 *
 * Mostly based on original driver:
 *
 * Copyright (C) 2005 Nokia Corporation
 * Author: Juha Yrjölä <juha.yrjola@nokia.com>
 */

#include <init.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/hw_random.h>
#include <clock.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/io.h>

#define RNG_REG_STATUS_RDY			(1 << 0)

#define RNG_REG_INTACK_RDY_MASK			(1 << 0)
#define RNG_REG_INTACK_SHUTDOWN_OFLO_MASK	(1 << 1)
#define RNG_SHUTDOWN_OFLO_MASK			(1 << 1)

#define RNG_CONTROL_STARTUP_CYCLES_SHIFT	16
#define RNG_CONTROL_STARTUP_CYCLES_MASK		(0xffff << 16)
#define RNG_CONTROL_ENABLE_TRNG_SHIFT		10
#define RNG_CONTROL_ENABLE_TRNG_MASK		(1 << 10)

#define RNG_CONFIG_MAX_REFIL_CYCLES_SHIFT	16
#define RNG_CONFIG_MAX_REFIL_CYCLES_MASK	(0xffff << 16)
#define RNG_CONFIG_MIN_REFIL_CYCLES_SHIFT	0
#define RNG_CONFIG_MIN_REFIL_CYCLES_MASK	(0xff << 0)

#define RNG_CONTROL_STARTUP_CYCLES		0xff
#define RNG_CONFIG_MIN_REFIL_CYCLES		0x21
#define RNG_CONFIG_MAX_REFIL_CYCLES		0x22

#define RNG_ALARMCNT_ALARM_TH_SHIFT		0x0
#define RNG_ALARMCNT_ALARM_TH_MASK		(0xff << 0)
#define RNG_ALARMCNT_SHUTDOWN_TH_SHIFT		16
#define RNG_ALARMCNT_SHUTDOWN_TH_MASK		(0x1f << 16)
#define RNG_ALARM_THRESHOLD			0xff
#define RNG_SHUTDOWN_THRESHOLD			0x4

#define RNG_REG_FROENABLE_MASK			0xffffff
#define RNG_REG_FRODETUNE_MASK			0xffffff

#define OMAP2_RNG_OUTPUT_SIZE			0x4
#define OMAP4_RNG_OUTPUT_SIZE			0x8
#define EIP76_RNG_OUTPUT_SIZE			0x10

/*
 * EIP76 RNG takes approx. 700us to produce 16 bytes of output data
 * as per testing results. And to account for the lack of udelay()'s
 * reliability, we keep the timeout as 1000us.
 */
#define RNG_DATA_FILL_TIMEOUT			100

enum {
	RNG_OUTPUT_0_REG = 0,
	RNG_OUTPUT_1_REG,
	RNG_OUTPUT_2_REG,
	RNG_OUTPUT_3_REG,
	RNG_STATUS_REG,
	RNG_INTMASK_REG,
	RNG_INTACK_REG,
	RNG_CONTROL_REG,
	RNG_CONFIG_REG,
	RNG_ALARMCNT_REG,
	RNG_FROENABLE_REG,
	RNG_FRODETUNE_REG,
	RNG_ALARMMASK_REG,
	RNG_ALARMSTOP_REG,
	RNG_REV_REG,
	RNG_SYSCONFIG_REG,
};

static const u16 reg_map_omap2[] = {
	[RNG_OUTPUT_0_REG]	= 0x0,
	[RNG_STATUS_REG]	= 0x4,
	[RNG_CONFIG_REG]	= 0x28,
	[RNG_REV_REG]		= 0x3c,
	[RNG_SYSCONFIG_REG]	= 0x40,
};

static const u16 reg_map_omap4[] = {
	[RNG_OUTPUT_0_REG]	= 0x0,
	[RNG_OUTPUT_1_REG]	= 0x4,
	[RNG_STATUS_REG]	= 0x8,
	[RNG_INTMASK_REG]	= 0xc,
	[RNG_INTACK_REG]	= 0x10,
	[RNG_CONTROL_REG]	= 0x14,
	[RNG_CONFIG_REG]	= 0x18,
	[RNG_ALARMCNT_REG]	= 0x1c,
	[RNG_FROENABLE_REG]	= 0x20,
	[RNG_FRODETUNE_REG]	= 0x24,
	[RNG_ALARMMASK_REG]	= 0x28,
	[RNG_ALARMSTOP_REG]	= 0x2c,
	[RNG_REV_REG]		= 0x1FE0,
	[RNG_SYSCONFIG_REG]	= 0x1FE4,
};

static const u16 reg_map_eip76[] = {
	[RNG_OUTPUT_0_REG]	= 0x0,
	[RNG_OUTPUT_1_REG]	= 0x4,
	[RNG_OUTPUT_2_REG]	= 0x8,
	[RNG_OUTPUT_3_REG]	= 0xc,
	[RNG_STATUS_REG]	= 0x10,
	[RNG_INTACK_REG]	= 0x10,
	[RNG_CONTROL_REG]	= 0x14,
	[RNG_CONFIG_REG]	= 0x18,
	[RNG_ALARMCNT_REG]	= 0x1c,
	[RNG_FROENABLE_REG]	= 0x20,
	[RNG_FRODETUNE_REG]	= 0x24,
	[RNG_ALARMMASK_REG]	= 0x28,
	[RNG_ALARMSTOP_REG]	= 0x2c,
	[RNG_REV_REG]		= 0x7c,
};

struct omap_rng_dev;
/**
 * struct omap_rng_pdata - RNG IP block-specific data
 * @regs: Pointer to the register offsets structure.
 * @data_size: No. of bytes in RNG output.
 * @data_present: Callback to determine if data is available.
 * @init: Callback for IP specific initialization sequence.
 * @cleanup: Callback for IP specific cleanup sequence.
 */
struct omap_rng_pdata {
	u16	*regs;
	u32	data_size;
	u32	(*data_present)(struct omap_rng_dev *priv);
	int	(*init)(struct omap_rng_dev *priv);
	void	(*cleanup)(struct omap_rng_dev *priv);
};

struct omap_rng_dev {
	void __iomem			*base;
	struct device			*dev;
	const struct omap_rng_pdata	*pdata;
	struct hwrng rng;
	struct clk			*clk;
	struct clk			*clk_reg;
};

static inline u32 omap_rng_read(struct omap_rng_dev *priv, u16 reg)
{
	return __raw_readl(priv->base + priv->pdata->regs[reg]);
}

static inline void omap_rng_write(struct omap_rng_dev *priv, u16 reg,
				      u32 val)
{
	__raw_writel(val, priv->base + priv->pdata->regs[reg]);
}


static int omap_rng_do_read(struct hwrng *rng, void *data, size_t max,
			    bool wait)
{
	struct omap_rng_dev *priv;
	int i, present;

	priv = (struct omap_rng_dev *)rng->priv;

	/* In Linux, max is always at least 32 bytes, which is greater than
	 * the 4 bytes required by the IP not to raise a data abort.
	 * In barebox, reading 4 bytes from a HWRNG is something we want
	 * support, so we check against 4 here and restrict memcpy_fromio
	 * size below.
	 */
	if (max < sizeof(u32))
		return -EFAULT;

	for (i = 0; i < RNG_DATA_FILL_TIMEOUT; i++) {
		present = priv->pdata->data_present(priv);
		if (present || !wait)
			break;

		udelay(10);
	}
	if (!present)
		return 0;

	max = min_t(size_t, max, priv->pdata->data_size);

	memcpy_fromio(data, priv->base + priv->pdata->regs[RNG_OUTPUT_0_REG], max);

	if (priv->pdata->regs[RNG_INTACK_REG])
		omap_rng_write(priv, RNG_INTACK_REG, RNG_REG_INTACK_RDY_MASK);

	return max;
}

static int omap_rng_init(struct hwrng *rng)
{
	struct omap_rng_dev *priv;

	priv = (struct omap_rng_dev *)rng->priv;
	return priv->pdata->init(priv);
}

static inline u32 omap2_rng_data_present(struct omap_rng_dev *priv)
{
	return omap_rng_read(priv, RNG_STATUS_REG) ? 0 : 1;
}

static int omap2_rng_init(struct omap_rng_dev *priv)
{
	omap_rng_write(priv, RNG_SYSCONFIG_REG, 0x1);
	return 0;
}

static void omap2_rng_cleanup(struct omap_rng_dev *priv)
{
	omap_rng_write(priv, RNG_SYSCONFIG_REG, 0x0);
}

static struct omap_rng_pdata omap2_rng_pdata = {
	.regs		= (u16 *)reg_map_omap2,
	.data_size	= OMAP2_RNG_OUTPUT_SIZE,
	.data_present	= omap2_rng_data_present,
	.init		= omap2_rng_init,
	.cleanup	= omap2_rng_cleanup,
};

static inline u32 omap4_rng_data_present(struct omap_rng_dev *priv)
{
	return omap_rng_read(priv, RNG_STATUS_REG) & RNG_REG_STATUS_RDY;
}

static int eip76_rng_init(struct omap_rng_dev *priv)
{
	u32 val;

	/* Return if RNG is already running. */
	if (omap_rng_read(priv, RNG_CONTROL_REG) & RNG_CONTROL_ENABLE_TRNG_MASK)
		return 0;

	/*  Number of 512 bit blocks of raw Noise Source output data that must
	 *  be processed by either the Conditioning Function or the
	 *  SP 800-90 DRBG ‘BC_DF’ functionality to yield a ‘full entropy’
	 *  output value.
	 */
	val = 0x5 << RNG_CONFIG_MIN_REFIL_CYCLES_SHIFT;

	/* Number of FRO samples that are XOR-ed together into one bit to be
	 * shifted into the main shift register
	 */
	val |= RNG_CONFIG_MAX_REFIL_CYCLES << RNG_CONFIG_MAX_REFIL_CYCLES_SHIFT;
	omap_rng_write(priv, RNG_CONFIG_REG, val);

	/* Enable TRNG */
	val = RNG_CONTROL_ENABLE_TRNG_MASK;
	omap_rng_write(priv, RNG_CONTROL_REG, val);

	return 0;
}

static int omap4_rng_init(struct omap_rng_dev *priv)
{
	u32 val;

	/* Return if RNG is already running. */
	if (omap_rng_read(priv, RNG_CONTROL_REG) & RNG_CONTROL_ENABLE_TRNG_MASK)
		return 0;

	val = RNG_CONFIG_MIN_REFIL_CYCLES << RNG_CONFIG_MIN_REFIL_CYCLES_SHIFT;
	val |= RNG_CONFIG_MAX_REFIL_CYCLES << RNG_CONFIG_MAX_REFIL_CYCLES_SHIFT;
	omap_rng_write(priv, RNG_CONFIG_REG, val);

	val = RNG_CONTROL_STARTUP_CYCLES << RNG_CONTROL_STARTUP_CYCLES_SHIFT;
	val |= RNG_CONTROL_ENABLE_TRNG_MASK;
	omap_rng_write(priv, RNG_CONTROL_REG, val);

	return 0;
}

static void omap4_rng_cleanup(struct omap_rng_dev *priv)
{
	int val;

	val = omap_rng_read(priv, RNG_CONTROL_REG);
	val &= ~RNG_CONTROL_ENABLE_TRNG_MASK;
	omap_rng_write(priv, RNG_CONTROL_REG, val);
}

static struct omap_rng_pdata omap4_rng_pdata = {
	.regs		= (u16 *)reg_map_omap4,
	.data_size	= OMAP4_RNG_OUTPUT_SIZE,
	.data_present	= omap4_rng_data_present,
	.init		= omap4_rng_init,
	.cleanup	= omap4_rng_cleanup,
};

static struct omap_rng_pdata eip76_rng_pdata = {
	.regs		= (u16 *)reg_map_eip76,
	.data_size	= EIP76_RNG_OUTPUT_SIZE,
	.data_present	= omap4_rng_data_present,
	.init		= eip76_rng_init,
	.cleanup	= omap4_rng_cleanup,
};

static const struct of_device_id omap_rng_of_match[] __maybe_unused = {
		{
			.compatible	= "ti,omap2-rng",
			.data		= &omap2_rng_pdata,
		},
		{
			.compatible	= "ti,omap4-rng",
			.data		= &omap4_rng_pdata,
		},
		{
			.compatible	= "inside-secure,safexcel-eip76",
			.data		= &eip76_rng_pdata,
		},
		{},
};
MODULE_DEVICE_TABLE(of, omap_rng_of_match);

static int of_get_omap_rng_device_details(struct omap_rng_dev *priv,
					  struct device *dev)
{
	priv->pdata = device_get_match_data(dev);
	if (!priv->pdata)
		return -ENODEV;

	return 0;
}

static struct clk *ti_sysc_clk_get_enabled(struct device *dev, const char *clk_id)
{
	struct clk *clk;

	clk = clk_get_optional_enabled(dev, clk_id);
	if (!clk)
		clk = clk_get_optional_enabled(dev->parent, clk_id);

	if (IS_ERR(clk))
		dev_errp_probe(dev, clk, "Unable to enable the clk\n");
	return clk;
}

static int omap_rng_probe(struct device *dev)
{
	struct omap_rng_dev *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(struct omap_rng_dev), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->rng.read = omap_rng_do_read;
	priv->rng.init = omap_rng_init;

	priv->rng.priv = (unsigned long)priv;
	dev->priv = priv;
	priv->dev = dev;

	priv->base = dev_platform_ioremap_resource(dev, 0);
	if (IS_ERR(priv->base)) {
		ret = PTR_ERR(priv->base);
		goto err_ioremap;
	}

	priv->rng.name = devm_kstrdup(dev, dev_name(dev), GFP_KERNEL);
	if (!priv->rng.name) {
		ret = -ENOMEM;
		goto err_ioremap;
	}

	priv->clk = ti_sysc_clk_get_enabled(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		goto err_ioremap;
	}

	priv->clk_reg = ti_sysc_clk_get_enabled(dev, "reg");
	if (IS_ERR(priv->clk_reg)) {
		ret = PTR_ERR(priv->clk_reg);
		goto err_ioremap;
	}

	ret = of_get_omap_rng_device_details(priv, dev);
	if (ret)
		goto err_register;

	ret = hwrng_register(dev, &priv->rng);
	if (ret)
		goto err_register;

	dev_info(dev, "Random Number Generator ver. %02x\n",
		 omap_rng_read(priv, RNG_REV_REG));

	return 0;

err_register:
	priv->base = NULL;

	clk_disable_unprepare(priv->clk_reg);
	clk_disable_unprepare(priv->clk);
err_ioremap:
	dev_err(dev, "initialization failed.\n");
	return ret;
}

static void omap_rng_remove(struct device *dev)
{
	struct omap_rng_dev *priv = dev->priv;


	priv->pdata->cleanup(priv);

	clk_disable_unprepare(priv->clk);
	clk_disable_unprepare(priv->clk_reg);
}

static struct driver omap_rng_driver = {
	.name		= "omap_rng",
	.of_match_table = of_match_ptr(omap_rng_of_match),
	.probe		= omap_rng_probe,
	.remove		= omap_rng_remove,
};

device_platform_driver(omap_rng_driver);
MODULE_ALIAS("platform:omap_rng");
MODULE_AUTHOR("Deepak Saxena (and others)");
MODULE_LICENSE("GPL");
