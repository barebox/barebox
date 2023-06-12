// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <init.h>
#include <of.h>
#include <regmap.h>
#include <linux/mfd/stpmic1.h>

static const struct regmap_config stpmic1_regmap_i2c_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xB3,
};

static int __init stpmic1_probe(struct device *dev)
{
	struct regmap *regmap;
	u32 reg;
	int ret;

	regmap = regmap_init_i2c(to_i2c_client(dev), &stpmic1_regmap_i2c_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = regmap_register_cdev(regmap, NULL);
	if (ret)
		return ret;

	ret = regmap_read(regmap, VERSION_SR, &reg);
	if (ret) {
		dev_err(dev, "Unable to read PMIC version\n");
		return ret;
	}
	dev_info(dev, "PMIC Chip Version: 0x%x\n", reg);

	return of_platform_populate(dev->of_node, NULL, dev);
}

static __maybe_unused struct of_device_id stpmic1_dt_ids[] = {
	{ .compatible = "st,stpmic1" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stpmic1_dt_ids);

static struct driver stpmic1_i2c_driver = {
	.name		= "stpmic1-i2c",
	.probe		= stpmic1_probe,
	.of_compatible	= DRV_OF_COMPAT(stpmic1_dt_ids),
};

coredevice_i2c_driver(stpmic1_i2c_driver);
