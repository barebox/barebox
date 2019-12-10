// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <init.h>
#include <malloc.h>
#include <of.h>
#include <regmap.h>
#include <xfuncs.h>
#include <linux/mfd/stpmic1.h>

struct stpmic1 {
	struct device_d		*dev;
	struct i2c_client	*client;
};

static int stpmic1_i2c_reg_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct stpmic1 *stpmic1 = ctx;
	u8 buf[1];
	int ret;

	ret = i2c_read_reg(stpmic1->client, reg, buf, 1);
	*val = buf[0];

	return ret == 1 ? 0 : ret;
}

static int stpmic1_i2c_reg_write(void *ctx, unsigned int reg, unsigned int val)
{
	struct stpmic1 *stpmic1 = ctx;
	u8 buf[] = {
		val & 0xff,
	};
	int ret;

	ret = i2c_write_reg(stpmic1->client, reg, buf, 1);

	return ret == 1 ? 0 : ret;
}

static struct regmap_bus regmap_stpmic1_i2c_bus = {
	.reg_write = stpmic1_i2c_reg_write,
	.reg_read = stpmic1_i2c_reg_read,
};

static const struct regmap_config stpmic1_regmap_i2c_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xB3,
};

static int __init stpmic1_probe(struct device_d *dev)
{
	struct stpmic1 *stpmic1;
	struct regmap *regmap;
	u32 reg;
	int ret;

	stpmic1 = xzalloc(sizeof(*stpmic1));
	stpmic1->dev = dev;

	stpmic1->client = to_i2c_client(dev);
	regmap = regmap_init(dev, &regmap_stpmic1_i2c_bus,
			     stpmic1, &stpmic1_regmap_i2c_config);

	ret = regmap_register_cdev(regmap, NULL);
	if (ret)
		return ret;

	ret = regmap_read(regmap, VERSION_SR, &reg);
	if (ret) {
		dev_err(dev, "Unable to read PMIC version\n");
		return ret;
	}
	dev_info(dev, "PMIC Chip Version: 0x%x\n", reg);

	return of_platform_populate(dev->device_node, NULL, dev);
}

static __maybe_unused struct of_device_id stpmic1_dt_ids[] = {
	{ .compatible = "st,stpmic1" },
	{ /* sentinel */ }
};

static struct driver_d stpmic1_i2c_driver = {
	.name		= "stpmic1-i2c",
	.probe		= stpmic1_probe,
	.of_compatible	= DRV_OF_COMPAT(stpmic1_dt_ids),
};

device_i2c_driver(stpmic1_i2c_driver);
