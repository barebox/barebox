// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (C) 2018 ROHM Semiconductors
//
// ROHM BD71837MWV and BD71847MWV PMIC driver
//
// Datasheet for BD71837MWV available from
// https://www.rohm.com/datasheet/BD71837MWV/bd71837mwv-e

#include <common.h>
#include <i2c/i2c.h>
#include <mfd/bd71837.h>
#include <linux/mfd/core.h>
#include <driver.h>
#include <poweroff.h>
#include <of.h>
#include <linux/regmap.h>

static struct mfd_cell bd71837_mfd_cells[] = {
	{ .name = "gpio-keys", },
	{ .name = "bd71837-clk", },
	{ .name = "bd71837-pmic", },
};

static struct mfd_cell bd71847_mfd_cells[] = {
	{ .name = "gpio-keys", },
	{ .name = "bd71847-clk", },
	{ .name = "bd71847-pmic", },
};

static const struct regmap_config bd718xx_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = BD718XX_MAX_REGISTER - 1,
};

static int bd718xx_init_press_duration(struct regmap *regmap,
				       struct device *dev)
{
	u32 short_press_ms, long_press_ms;
	u32 short_press_value, long_press_value;
	int ret;

	ret = of_property_read_u32(dev->of_node, "rohm,short-press-ms",
				   &short_press_ms);
	if (!ret) {
		short_press_value = min(15u, (short_press_ms + 250) / 500);
		ret = regmap_update_bits(regmap, BD718XX_PWRONCONFIG0,
					 BD718XX_PWRBTN_PRESS_DURATION_MASK,
					 short_press_value);
		if (ret) {
			dev_err(dev, "Failed to init pwron short press\n");
			return ret;
		}
	}

	ret = of_property_read_u32(dev->of_node, "rohm,long-press-ms",
				   &long_press_ms);
	if (!ret) {
		long_press_value = min(15u, (long_press_ms + 500) / 1000);
		ret = regmap_update_bits(regmap, BD718XX_PWRONCONFIG1,
					 BD718XX_PWRBTN_PRESS_DURATION_MASK,
					 long_press_value);
		if (ret) {
			dev_err(dev, "Failed to init pwron long press\n");
			return ret;
		}
	}

	return 0;
}

static int bd718xx_i2c_probe(struct device *dev)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct regmap *regmap;
	int ret;
	unsigned int chip_type;
	struct mfd_cell *mfd;
	int cells;

	chip_type = (unsigned int)(uintptr_t)device_get_match_data(dev);
	switch (chip_type) {
	case ROHM_CHIP_TYPE_BD71837:
		mfd = bd71837_mfd_cells;
		cells = ARRAY_SIZE(bd71837_mfd_cells);
		break;
	case ROHM_CHIP_TYPE_BD71847:
		mfd = bd71847_mfd_cells;
		cells = ARRAY_SIZE(bd71847_mfd_cells);
		break;
	default:
		dev_err(dev, "Unknown device type");
		return -EINVAL;
	}

	regmap = regmap_init_i2c(i2c, &bd718xx_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(dev, "regmap initialization failed\n");
		return PTR_ERR(regmap);
	}

	ret = bd718xx_init_press_duration(regmap, dev);
	if (ret)
		return ret;

	ret = mfd_add_devices(dev, mfd, cells);
	if (ret)
		dev_err(dev, "Failed to create subdevices\n");

	return regmap_register_cdev(regmap, NULL);
}

static const struct of_device_id bd718xx_of_match[] = {
	{
		.compatible = "rohm,bd71837",
		.data = (void *)ROHM_CHIP_TYPE_BD71837,
	},
	{
		.compatible = "rohm,bd71847",
		.data = (void *)ROHM_CHIP_TYPE_BD71847,
	},
	{
		.compatible = "rohm,bd71850",
		.data = (void *)ROHM_CHIP_TYPE_BD71847,
	},
	{ }
};
MODULE_DEVICE_TABLE(of, bd718xx_of_match);

static struct driver bd718xx_i2c_driver = {
	.name = "rohm-bd718x7",
	.of_match_table = bd718xx_of_match,
	.probe = bd718xx_i2c_probe,
};
coredevice_i2c_driver(bd718xx_i2c_driver);

MODULE_AUTHOR("Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>");
MODULE_DESCRIPTION("ROHM BD71837/BD71847 Power Management IC driver");
MODULE_LICENSE("GPL");
