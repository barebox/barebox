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
#include <linux/regmap.h>
#include <mach/stm32mp/bsec.h>
#include <machine_id.h>
#include <linux/nvmem-provider.h>

#include "stm32-bsec-optee-ta.h"

#define BSEC_OTP_SERIAL	13

struct bsec_priv {
	struct device dev;
	struct regmap_config map_config;
	int permanent_write_enable;
	u8 lower;
	struct tee_context *ctx;
};

struct stm32_bsec_data {
	size_t size;
	u8 lower;
	bool ta;
};

static int bsec_smc(enum bsec_op op, u32 field,
		    unsigned data2, unsigned *val)
{
	enum bsec_smc ret = stm32mp_smc(STM32_SMC_BSEC, op, field / 4, data2, val);
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
	return bsec_smc(BSEC_SMC_READ_SHADOW, reg, 0, val);
}

static int stm32_bsec_reg_write(void *ctx, unsigned reg, unsigned val)
{
	struct bsec_priv *priv = ctx;

	if (priv->permanent_write_enable)
		return bsec_smc(BSEC_SMC_PROG_OTP, reg, val, NULL);
	else
		return bsec_smc(BSEC_SMC_WRITE_SHADOW, reg, val, NULL);
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
			       unique_id, sizeof(unique_id) / 4);
	if (ret)
		return;

	machine_id_set_hashable(unique_id, sizeof(unique_id));
}

static int stm32_bsec_read_mac(struct bsec_priv *priv, int offset, u8 *mac)
{
	u32 val[2];
	int ret;

	if (priv->ctx) {
		ret = stm32_bsec_optee_ta_read(priv->ctx, offset * 4, val, sizeof(val));
	} else {
		/* Some TF-A does not copy all of OTP into shadow registers, so make
		 * sure we read the _real_ OTP bits here.
		 */
		ret = bsec_smc(BSEC_SMC_READ_OTP, offset * 4, 0, &val[0]);
		if (!ret)
			ret = bsec_smc(BSEC_SMC_READ_OTP, offset * 4 + 4, 0, &val[1]);
	}

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

static int stm32_bsec_pta_read(void *context, unsigned int offset, unsigned int *val)
{
	struct bsec_priv *priv = context;

	return stm32_bsec_optee_ta_read(priv->ctx, offset, val, sizeof(val));
}

static int stm32_bsec_pta_write(void *context, unsigned int offset, unsigned int val)
{
	struct bsec_priv *priv = context;

	if (!priv->permanent_write_enable)
		return -EACCES;

	return stm32_bsec_optee_ta_write(priv->ctx, priv->lower, offset, &val, sizeof(val));
}

static struct regmap_bus stm32_bsec_optee_regmap_bus = {
	.reg_write = stm32_bsec_pta_write,
	.reg_read = stm32_bsec_pta_read,
};

static bool stm32_bsec_smc_check(void)
{
	u32 val;
	int ret;

	/* check that the OP-TEE support the BSEC SMC (legacy mode) */
	ret = bsec_smc(BSEC_SMC_READ_SHADOW, 0, 0, &val);

	return !ret;
}

static bool optee_presence_check(void)
{
	struct device_node *np;
	bool tee_detected = false;

	/* check that the OP-TEE node is present and available. */
	np = of_find_compatible_node(NULL, NULL, "linaro,optee-tz");
	if (np && of_device_is_available(np))
		tee_detected = true;
	of_node_put(np);

	return tee_detected;
}

static int stm32_bsec_probe(struct device *dev)
{
	const struct regmap_bus *regmap_bus;
	struct regmap *map;
	struct bsec_priv *priv;
	int ret = 0;
	const struct stm32_bsec_data *data;
	struct nvmem_device *nvmem;

	ret = dev_get_drvdata(dev, (const void **)&data);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));

	dev_set_name(&priv->dev, "bsec");
	priv->dev.parent = dev;
	register_device(&priv->dev);

	priv->map_config.reg_bits = 32;
	priv->map_config.val_bits = 32;
	priv->map_config.reg_stride = 4;
	priv->map_config.max_register = (data->size / 4) - 1;

	priv->lower = data->lower;

	if (data->ta || optee_presence_check()) {
		ret = stm32_bsec_optee_ta_open(&priv->ctx);
		if (ret) {
			/* wait for OP-TEE client driver to be up and ready */
			if (ret == -EPROBE_DEFER)
				return -EPROBE_DEFER;
			/* BSEC PTA is required or SMC not supported */
			if (data->ta || !stm32_bsec_smc_check())
				return ret;
		}
	}

	if (priv->ctx)
		regmap_bus = &stm32_bsec_optee_regmap_bus;
	else
		regmap_bus = &stm32_bsec_regmap_bus;

	map = regmap_init(dev, regmap_bus, priv, &priv->map_config);
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

	dev_dbg(dev, "using %s API\n", priv->ctx ? "OP-TEE" : "SiP");

	return 0;
}

/*
 * STM32MP15/13 BSEC OTP regions: 4096 OTP bits (with 3072 effective bits)
 * => 96 x 32-bits data words
 * - Lower: 1K bits, 2:1 redundancy, incremental bit programming
 *   => 32 (x 32-bits) lower shadow registers = words 0 to 31
 * - Upper: 2K bits, ECC protection, word programming only
 *   => 64 (x 32-bits) = words 32 to 95
 */
static struct stm32_bsec_data stm32mp15_bsec_data = {
	.size = 384,
	.lower = 32,
	.ta = false,
};

static const struct stm32_bsec_data stm32mp13_bsec_data = {
	.size = 384,
	.lower = 32,
	.ta = true,
};

static __maybe_unused struct of_device_id stm32_bsec_dt_ids[] = {
	{ .compatible = "st,stm32mp15-bsec", .data = &stm32mp15_bsec_data },
	{ .compatible = "st,stm32mp13-bsec", .data = &stm32mp13_bsec_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stm32_bsec_dt_ids);

static struct driver stm32_bsec_driver = {
	.name	= "stm32_bsec",
	.probe	= stm32_bsec_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_bsec_dt_ids),
};
postcore_platform_driver(stm32_bsec_driver);
