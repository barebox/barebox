// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, Ahmad Fatoum, Pengutronix
 */

#include <i2c/i2c.h>
#include <regmap.h>


static int regmap_i2c_reg_read(void *client, unsigned int reg, unsigned int *val)
{
	u8 buf[1];
	int ret;

	ret = i2c_read_reg(client, reg, buf, 1);
	if (ret != 1)
		return ret;

	*val = buf[0];
	return 0;
}

static int regmap_i2c_reg_write(void *client, unsigned int reg, unsigned int val)
{
	u8 buf[] = { val & 0xff };
	int ret;

	ret = i2c_write_reg(client, reg, buf, 1);
	if (ret != 1)
		return ret;

	return 0;
}

static const struct regmap_bus regmap_regmap_i2c_bus = {
	.reg_write = regmap_i2c_reg_write,
	.reg_read = regmap_i2c_reg_read,
};

struct regmap *regmap_init_i2c(struct i2c_client *client,
			       const struct regmap_config *config)
{
	return  regmap_init(&client->dev, &regmap_regmap_i2c_bus, client, config);
}

static int regmap_smbus_byte_reg_read(void *client, unsigned int reg, unsigned int *val)
{
	int ret;

	if (reg > 0xff)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;

	*val = ret;

	return 0;
}

static int regmap_smbus_byte_reg_write(void *client, unsigned int reg, unsigned int val)
{
	if (val > 0xff || reg > 0xff)
		return -EINVAL;

	return i2c_smbus_write_byte_data(client, reg, val);
}

static const struct regmap_bus regmap_smbus_byte = {
	.reg_write = regmap_smbus_byte_reg_write,
	.reg_read = regmap_smbus_byte_reg_read,
};

struct regmap *regmap_init_i2c_smbus(struct i2c_client *client,
			       const struct regmap_config *config)
{
	if (config->val_bits != 8 || config->reg_bits != 8)
		return ERR_PTR(-ENOSYS);
	return regmap_init(&client->dev, &regmap_smbus_byte, client, config);
}
