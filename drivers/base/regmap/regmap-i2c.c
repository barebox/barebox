// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, Ahmad Fatoum, Pengutronix
 */

#include <i2c/i2c.h>
#include <linux/regmap.h>


static int regmap_i2c_read(void *context,
			   const void *reg, size_t reg_size,
			   void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg xfer[2];
	int ret;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = reg_size;
	xfer[0].buf = (void *)reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = val_size;
	xfer[1].buf = val;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret == 2)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int regmap_i2c_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;

	ret = i2c_master_send(i2c, data, count);
	if (ret == count)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static const struct regmap_bus regmap_regmap_i2c_bus = {
	.write = regmap_i2c_write,
	.read = regmap_i2c_read,
	.reg_format_endian_default = REGMAP_ENDIAN_BIG,
	.val_format_endian_default = REGMAP_ENDIAN_BIG,
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
