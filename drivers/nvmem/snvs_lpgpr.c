/*
 * Copyright (c) 2015 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 * Copyright (c) 2017 Pengutronix, Oleksij Rempel <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_device.h>
#include <malloc.h>
#include <regmap.h>
#include <mfd/syscon.h>
#include <linux/nvmem-provider.h>

#define IMX6Q_SNVS_HPLR		0x00
#define IMX6Q_GPR_SL		BIT(5)
#define IMX6Q_SNVS_LPLR		0x34
#define IMX6Q_GPR_HL		BIT(5)
#define IMX6Q_SNVS_LPGPR	0x68

struct snvs_lpgpr_cfg {
	int offset;
	int offset_hplr;
	int offset_lplr;
};

struct snvs_lpgpr_priv {
	struct device_d			*dev;
	struct regmap			*regmap;
	struct nvmem_config		cfg;
	const struct snvs_lpgpr_cfg	*dcfg;
};

static const struct snvs_lpgpr_cfg snvs_lpgpr_cfg_imx6q = {
	.offset		= IMX6Q_SNVS_LPGPR,
	.offset_hplr	= IMX6Q_SNVS_HPLR,
	.offset_lplr	= IMX6Q_SNVS_LPLR,
};

static int snvs_lpgpr_write(struct device_d *dev, const int offset,
			    const void *val, int bytes)
{
	struct snvs_lpgpr_priv *priv = dev->parent->priv;
	const struct snvs_lpgpr_cfg *dcfg = priv->dcfg;
	unsigned int lock_reg;
	int ret;

	ret = regmap_read(priv->regmap, dcfg->offset_hplr, &lock_reg);
	if (ret < 0)
		return ret;

	if (lock_reg & IMX6Q_GPR_SL)
		return -EPERM;

	ret = regmap_read(priv->regmap, dcfg->offset_lplr, &lock_reg);
	if (ret < 0)
		return ret;

	if (lock_reg & IMX6Q_GPR_HL)
		return -EPERM;

	return regmap_bulk_write(priv->regmap, dcfg->offset + offset, val,
				 bytes);
}

static int snvs_lpgpr_read(struct device_d *dev, const int offset, void *val,
			   int bytes)
{
	struct snvs_lpgpr_priv *priv = dev->parent->priv;
	const struct snvs_lpgpr_cfg *dcfg = priv->dcfg;

	return regmap_bulk_read(priv->regmap, dcfg->offset + offset,
				val, bytes);
}

static const struct nvmem_bus snvs_lpgpr_nvmem_bus = {
	.write = snvs_lpgpr_write,
	.read  = snvs_lpgpr_read,
};

static int snvs_lpgpr_probe(struct device_d *dev)
{
	struct device_node *node = dev->device_node;
	struct device_node *syscon_node;
	struct snvs_lpgpr_priv *priv;
	struct nvmem_config *cfg;
	struct nvmem_device *nvmem;

	if (!node)
		return -ENOENT;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dcfg = of_device_get_match_data(dev);
	if (!priv->dcfg)
		return -EINVAL;

	syscon_node = of_get_parent(node);
	if (!syscon_node)
		return -ENODEV;

	priv->regmap = syscon_node_to_regmap(syscon_node);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	cfg = &priv->cfg;
	cfg->name = dev_name(dev);
	cfg->dev = dev;
	cfg->stride = 4,
	cfg->word_size = 4,
	cfg->size = 4,
	cfg->bus = &snvs_lpgpr_nvmem_bus,

	nvmem = nvmem_register(cfg);
	if (IS_ERR(nvmem)) {
		free(priv);
		return PTR_ERR(nvmem);
	}

	dev->priv = priv;

	return 0;
}

static __maybe_unused struct of_device_id snvs_lpgpr_dt_ids[] = {
	{ .compatible = "fsl,imx6q-snvs-lpgpr", .data = &snvs_lpgpr_cfg_imx6q },
	{ .compatible = "fsl,imx6ul-snvs-lpgpr", .data = &snvs_lpgpr_cfg_imx6q },
	{ },
};

static struct driver_d snvs_lpgpr_driver = {
	.name	= "snvs_lpgpr",
	.probe	= snvs_lpgpr_probe,
	.of_compatible = DRV_OF_COMPAT(snvs_lpgpr_dt_ids),
};
device_platform_driver(snvs_lpgpr_driver);
