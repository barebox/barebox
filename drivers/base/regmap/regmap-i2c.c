// SPDX-License-Identifier: GPL-2.0
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
