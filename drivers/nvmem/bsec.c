// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <net.h>
#include <io.h>
#include <of.h>
#include <regmap.h>
#include <mach/bsec.h>
#include <machine_id.h>
#include <linux/nvmem-provider.h>

#define BSEC_OTP_SERIAL	13

struct bsec_priv {
	struct regmap *map;
	u32 svc_id;
	struct device_d dev;
	struct regmap_config map_config;
	struct nvmem_config config;
};

struct stm32_bsec_data {
	unsigned long svc_id;
	int num_regs;
};

static int bsec_smc(struct bsec_priv *priv, u8 op, enum bsec_field field,
		    unsigned data2, unsigned *val)
{
	enum bsec_smc ret = stm32mp_smc(priv->svc_id, op, field / 4, data2, val);
	switch(ret)
	{
	case BSEC_SMC_OK:
		return 0;
	case BSEC_SMC_ERROR:
	case BSEC_SMC_DISTURBED:
	case BSEC_SMC_PROG_FAIL:
	case BSEC_SMC_LOCK_FAIL:
	case BSEC_SMC_WRITE_FAIL:
	case BSEC_SMC_SHADOW_FAIL:
		return -EIO;
	case BSEC_SMC_INVALID_PARAM:
		return -EINVAL;
	case BSEC_SMC_TIMEOUT:
		return -ETIMEDOUT;
	}

	return -ENXIO;
}

static int stm32_bsec_read_shadow(void *ctx, unsigned reg, unsigned *val)
{
	return bsec_smc(ctx, BSEC_SMC_READ_SHADOW, reg, 0, val);
}

static int stm32_bsec_reg_write_shadow(void *ctx, unsigned reg, unsigned val)
{
	return bsec_smc(ctx, BSEC_SMC_WRITE_SHADOW, reg, val, NULL);
}

static struct regmap_bus stm32_bsec_regmap_bus = {
	.reg_write = stm32_bsec_reg_write_shadow,
	.reg_read = stm32_bsec_read_shadow,
};

static int stm32_bsec_write(struct device_d *dev, int offset,
			    const void *val, int bytes)
{
	struct bsec_priv *priv = dev->parent->priv;

	return regmap_bulk_write(priv->map, offset, val, bytes);
}

static int stm32_bsec_read(struct device_d *dev, int offset,
			   void *val, int bytes)
{
	struct bsec_priv *priv = dev->parent->priv;

	return regmap_bulk_read(priv->map, offset, val, bytes);
}

static const struct nvmem_bus stm32_bsec_nvmem_bus = {
	.write = stm32_bsec_write,
	.read  = stm32_bsec_read,
};

static void stm32_bsec_set_unique_machine_id(struct regmap *map)
{
	u32 unique_id[3];
	int ret;

	ret = regmap_bulk_read(map, BSEC_OTP_SERIAL * 4,
			       unique_id, sizeof(unique_id));
	if (ret)
		return;

	machine_id_set_hashable(unique_id, sizeof(unique_id));
}

static int stm32_bsec_read_mac(struct regmap *map, int offset, u8 *mac)
{
	u8 res[8];
	int ret;

	ret = regmap_bulk_read(map, offset * 4, res, 8);
	if (ret)
		return ret;

	memcpy(mac, res, ETH_ALEN);
	return 0;
}

static void stm32_bsec_init_dt(struct bsec_priv *priv)
{
	struct device_node *node = priv->dev.parent->device_node;
	struct device_node *rnode;
	u32 phandle, offset;
	char mac[ETH_ALEN];
	const __be32 *prop;

	int len;
	int ret;

	if (!node)
		return;

	prop = of_get_property(node, "barebox,provide-mac-address", &len);
	if (!prop)
		return;

	if (len != 2 * sizeof(__be32))
		return;

	phandle = be32_to_cpup(prop++);

	rnode = of_find_node_by_phandle(phandle);
	offset = be32_to_cpup(prop++);

	ret = stm32_bsec_read_mac(priv->map, offset, mac);
	if (ret) {
		dev_warn(&priv->dev, "error setting MAC address: %s\n",
			 strerror(-ret));
		return;
	}

	of_eth_register_ethaddr(rnode, mac);
}

static int stm32_bsec_probe(struct device_d *dev)
{
	struct bsec_priv *priv;
	int ret = 0;
	const struct stm32_bsec_data *data;
	struct nvmem_device *nvmem;

	ret = dev_get_drvdata(dev, (const void **)&data);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));

	priv->svc_id = data->svc_id;

	dev_set_name(&priv->dev, "bsec");
	priv->dev.parent = dev;
	register_device(&priv->dev);

	priv->map_config.reg_bits = 32;
	priv->map_config.val_bits = 32;
	priv->map_config.reg_stride = 4;
	priv->map_config.max_register = data->num_regs;

	priv->map = regmap_init(dev, &stm32_bsec_regmap_bus, priv, &priv->map_config);
	if (IS_ERR(priv->map))
		return PTR_ERR(priv->map);

	priv->config.name = "stm32-bsec";
	priv->config.dev = dev;
	priv->config.stride = 4;
	priv->config.word_size = 4;
	priv->config.size = data->num_regs;
	priv->config.bus = &stm32_bsec_nvmem_bus;
	dev->priv = priv;

	nvmem = nvmem_register(&priv->config);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	if (IS_ENABLED(CONFIG_MACHINE_ID))
		stm32_bsec_set_unique_machine_id(priv->map);

	stm32_bsec_init_dt(priv);

	return 0;
}

static struct stm32_bsec_data stm32mp15_bsec_data = {
	.num_regs = 95 * 4,
	.svc_id = STM32_SMC_BSEC,
};

static __maybe_unused struct of_device_id stm32_bsec_dt_ids[] = {
	{ .compatible = "st,stm32mp15-bsec", .data = &stm32mp15_bsec_data },
	{ /* sentinel */ }
};

static struct driver_d stm32_bsec_driver = {
	.name	= "stm32_bsec",
	.probe	= stm32_bsec_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_bsec_dt_ids),
};
postcore_platform_driver(stm32_bsec_driver);
