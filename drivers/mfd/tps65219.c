// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for TI TPS65219
 */

#define pr_fmt(fmt) "tps65219: " fmt

#include <i2c/i2c.h>
#include <linux/mfd/core.h>
#include <driver.h>
#include <of.h>
#include <linux/regmap.h>
#include <linux/mfd/tps65219.h>
#include <linux/device.h>

static const struct mfd_cell tps65219_cells[] = {
	{ .name = "tps65219-regulator", },
	{ .name = "tps65219-gpio", },
};

static const struct mfd_cell tps65219_pwrbutton_cell = {
	.name = "tps65219-pwrbutton",
};

static const struct regmap_config tps65219_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = TPS65219_REG_FACTORY_CONFIG_2,
};

static int tps65219_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tps65219 *tps;
	unsigned int chipid;
	bool pwr_button;
	int ret;

	tps = devm_kzalloc(&client->dev, sizeof(*tps), GFP_KERNEL);
	if (!tps)
		return -ENOMEM;

	i2c_set_clientdata(client, tps);

	tps->dev = &client->dev;

	tps->regmap = regmap_init_i2c(client, &tps65219_regmap_config);
	if (IS_ERR(tps->regmap))
		return dev_err_probe(tps->dev, PTR_ERR(tps->regmap), "Failed to allocate register map\n");

	ret = regmap_read(tps->regmap, TPS65219_REG_TI_DEV_ID, &chipid);
	if (ret)
		return dev_err_probe(tps->dev, ret, "Failed to read device ID\n");

	dev->priv = tps;

	ret = mfd_add_devices(tps->dev, tps65219_cells, ARRAY_SIZE(tps65219_cells));
	if (ret)
		return dev_err_probe(tps->dev, ret, "Failed to add child devices\n");

	pwr_button = of_property_read_bool(tps->dev->of_node, "ti,power-button");
	if (pwr_button) {
		ret = mfd_add_devices(tps->dev, &tps65219_pwrbutton_cell, 1);
		if (ret)
			return dev_err_probe(tps->dev, ret, "Failed to add power-button\n");
	}

	return 0;
}

static const struct of_device_id tps65219_of_match[] = {
	{ .compatible = "ti,tps65219" },
	{ },
};
MODULE_DEVICE_TABLE(of, tps65219_of_match);

static __maybe_unused struct driver tps65219_i2c_driver = {
	.name = "tps65219",
	.of_compatible = tps65219_of_match,
	.probe    = tps65219_probe,
};
coredevice_i2c_driver(tps65219_i2c_driver);
