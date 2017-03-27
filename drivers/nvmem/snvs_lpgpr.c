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
#include <malloc.h>
#include <regmap.h>
#include <mfd/syscon.h>
#include <linux/nvmem-provider.h>

struct snvs_lpgpr_priv {
	struct device_d		*dev;
	struct regmap		*regmap;
	int			offset;
	struct nvmem_config	cfg;
};

static int snvs_lpgpr_write(struct device_d *dev, const int reg,
			    const void *_val, int bytes)
{
	struct snvs_lpgpr_priv *priv = dev->parent->priv;
	const u32 *val = _val;
	int i = 0, words = bytes / 4;

	while (words--)
		regmap_write(priv->regmap, priv->offset + reg + (i++ * 4),
			     *val++);

	return 0;
}

static int snvs_lpgpr_read(struct device_d *dev, const int reg, void *_val,
			int bytes)
{
	struct snvs_lpgpr_priv *priv = dev->parent->priv;
	u32 *val = _val;
	int i = 0, words = bytes / 4;

	while (words--)
		regmap_read(priv->regmap, priv->offset + reg + (i++ * 4),
			    val++);


	return 0;
}

static const struct nvmem_bus snvs_lpgpr_nvmem_bus = {
	.write = snvs_lpgpr_write,
	.read  = snvs_lpgpr_read,
};

static int snvs_lpgpr_probe(struct device_d *dev)
{
	struct device_node *node = dev->device_node;
	struct snvs_lpgpr_priv *priv;
	struct nvmem_config *cfg;
	struct nvmem_device *nvmem;
	int err;

	if (!node)
		return -ENOENT;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->regmap = syscon_node_to_regmap(of_get_parent(node));
	if (IS_ERR(priv->regmap)) {
		free(priv);
		return PTR_ERR(priv->regmap);
	}

	err = of_property_read_u32(node, "offset", &priv->offset);
	if (err)
		return err;

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
	{ .compatible = "fsl,imx6sl-snvs-lpgpr", },
	{ .compatible = "fsl,imx6q-snvs-lpgpr", },
	{ },
};

static struct driver_d snvs_lpgpr_driver = {
	.name	= "nvmem-snvs-lpgpr",
	.probe	= snvs_lpgpr_probe,
	.of_compatible = DRV_OF_COMPAT(snvs_lpgpr_dt_ids),
};
device_platform_driver(snvs_lpgpr_driver);
