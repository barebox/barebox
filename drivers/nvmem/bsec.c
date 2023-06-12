// SPDX-License-Identifier: GPL-2.0-only
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
#include <mach/stm32mp/bsec.h>
#include <machine_id.h>
#include <linux/nvmem-provider.h>

#define BSEC_OTP_SERIAL	13

struct bsec_priv {
	struct device dev;
	u32 svc_id;
	struct regmap_config map_config;
	int permanent_write_enable;
};

struct stm32_bsec_data {
	unsigned long svc_id;
	int num_regs;
};

static int bsec_smc(struct bsec_priv *priv, enum bsec_op op, u32 field,
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

static int stm32_bsec_reg_write(void *ctx, unsigned reg, unsigned val)
{
	struct bsec_priv *priv = ctx;

	if (priv->permanent_write_enable)
		return bsec_smc(ctx, BSEC_SMC_PROG_OTP, reg, val, NULL);
	else
		return bsec_smc(ctx, BSEC_SMC_WRITE_SHADOW, reg, val, NULL);
}

static struct regmap_bus stm32_bsec_regmap_bus = {
	.reg_write = stm32_bsec_reg_write,
	.reg_read = stm32_bsec_read_shadow,
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

static int stm32_bsec_read_mac(struct bsec_priv *priv, int offset, u8 *mac)
{
	u32 val[2];
	int ret;

	/* Some TF-A does not copy all of OTP into shadow registers, so make
	 * sure we read the _real_ OTP bits here.
	 */
	ret = bsec_smc(priv, BSEC_SMC_READ_OTP, offset * 4, 0, &val[0]);
	if (ret)
		return ret;
	ret = bsec_smc(priv, BSEC_SMC_READ_OTP, offset * 4 + 4, 0, &val[1]);
	if (ret)
		return ret;

	memcpy(mac, val, ETH_ALEN);
	return 0;
}

static void stm32_bsec_init_dt(struct bsec_priv *priv, struct device *dev,
			       struct regmap *map)
{
	struct device_node *node = dev->of_node;
	struct device_node *rnode;
	u32 phandle, offset;
	char mac[ETH_ALEN];
	const __be32 *prop;

	int len;
	int ret;

	prop = of_get_property(node, "barebox,provide-mac-address", &len);
	if (!prop)
		return;

	if (len != 2 * sizeof(__be32))
		return;

	phandle = be32_to_cpup(prop++);

	rnode = of_find_node_by_phandle(phandle);
	offset = be32_to_cpup(prop++);

	ret = stm32_bsec_read_mac(priv, offset, mac);
	if (ret) {
		dev_warn(dev, "error setting MAC address: %s\n", strerror(-ret));
		return;
	}

	of_eth_register_ethaddr(rnode, mac);
}

static int stm32_bsec_probe(struct device *dev)
{
	struct regmap *map;
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

	map = regmap_init(dev, &stm32_bsec_regmap_bus, priv, &priv->map_config);
	if (IS_ERR(map))
		return PTR_ERR(map);

	if (IS_ENABLED(CONFIG_STM32_BSEC_WRITE)) {
		dev_add_param_bool(&priv->dev, "permanent_write_enable",
				NULL, NULL, &priv->permanent_write_enable, NULL);
	}

	nvmem = nvmem_regmap_register(map, "stm32-bsec");
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	if (IS_ENABLED(CONFIG_MACHINE_ID))
		stm32_bsec_set_unique_machine_id(map);

	stm32_bsec_init_dt(priv, dev, map);

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
MODULE_DEVICE_TABLE(of, stm32_bsec_dt_ids);

static struct driver stm32_bsec_driver = {
	.name	= "stm32_bsec",
	.probe	= stm32_bsec_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_bsec_dt_ids),
};
postcore_platform_driver(stm32_bsec_driver);
