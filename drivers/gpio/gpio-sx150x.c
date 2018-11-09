/*
 *  Driver for SX150x I2C GPIO expanders
 *
 *  This code was ported from linux-4.9 kernel driver by
 *  Andrey Smirnov <andrew.smirnov@gmail.com>.
 *
 *  Orginal code with it's copyright info can be found in
 *  drivers/pinctrl/pinctrl-sx150x.c
 *
 *  Note: That although linux driver was converted from being a GPIO
 *  subsystem to Pinctrl subsytem driver, due to Barebox's lack of
 *  similar provisions this driver is still a GPIO driver.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <regmap.h>

#include <gpio.h>
#include <of_device.h>

enum {
	SX150X_123 = 0,
	SX150X_456,
	SX150X_789,
};

enum {
	SX150X_MAX_REGISTER = 0xad,
};

struct sx150x_device_data {
	u8 model;
	u8 reg_dir;
	u8 reg_data;
	u8 ngpios;
};

struct sx150x_gpio {
	struct device *dev;
	struct i2c_client *client;
	struct gpio_chip gpio;
	struct regmap *regmap;
	const struct sx150x_device_data *data;
};

static const struct sx150x_device_data sx1503q_device_data = {
	.model    = SX150X_123,
	.reg_dir  = 0x02,
	.reg_data = 0x00,
	.ngpios	  = 16,
};

static struct sx150x_gpio *to_sx150x_gpio(struct gpio_chip *gpio)
{
	return container_of(gpio, struct sx150x_gpio, gpio);
}

static int sx150x_gpio_get_direction(struct gpio_chip *gpio,
				      unsigned int offset)
{
	struct sx150x_gpio *sx150x = to_sx150x_gpio(gpio);
	unsigned int value;
	int ret;

	ret = regmap_read(sx150x->regmap, sx150x->data->reg_dir, &value);
	if (ret < 0)
		return ret;

	return !!(value & BIT(offset));
}

static int sx150x_gpio_get(struct gpio_chip *gpio, unsigned int offset)
{
	struct sx150x_gpio *sx150x = to_sx150x_gpio(gpio);
	unsigned int value;
	int ret;

	ret = regmap_read(sx150x->regmap, sx150x->data->reg_data, &value);
	if (ret < 0)
		return ret;

	return !!(value & BIT(offset));
}

static int __sx150x_gpio_set(struct sx150x_gpio *sx150x, unsigned int offset,
			     int value)
{
	return regmap_write_bits(sx150x->regmap, sx150x->data->reg_data,
				 BIT(offset), value ? BIT(offset) : 0);
}


static void sx150x_gpio_set(struct gpio_chip *gpio, unsigned int offset,
			    int value)
{
	__sx150x_gpio_set(to_sx150x_gpio(gpio), offset, value);
}

static int sx150x_gpio_direction_input(struct gpio_chip *gpio,
				       unsigned int offset)
{
	struct sx150x_gpio *sx150x = to_sx150x_gpio(gpio);

	return regmap_write_bits(sx150x->regmap,
				 sx150x->data->reg_dir,
				 BIT(offset), BIT(offset));
}

static int sx150x_gpio_direction_output(struct gpio_chip *gpio,
				       unsigned int offset, int value)
{
	struct sx150x_gpio *sx150x = to_sx150x_gpio(gpio);
	int ret;

	ret = __sx150x_gpio_set(sx150x, offset, value);
	if (ret < 0)
		return ret;

	return regmap_write_bits(sx150x->regmap,
				 sx150x->data->reg_dir,
				 BIT(offset), 0);
}

static int sx150x_regmap_reg_width(struct sx150x_gpio *sx150x,
				   unsigned int reg)
{
	return sx150x->data->ngpios;
}

/*
 * In order to mask the differences between 16 and 8 bit expander
 * devices we set up a sligthly ficticious regmap that pretends to be
 * a set of 16-bit registers and transparently reconstructs those
 * registers via multiple I2C/SMBus reads
 *
 * This way the rest of the driver code, interfacing with the chip via
 * regmap API, can work assuming that each GPIO pin is represented by
 * a group of bits at an offset proportioan to GPIO number within a
 * given register.
 *
 */
static int sx150x_regmap_reg_read(void *context, unsigned int reg,
				  unsigned int *result)
{
	int ret, n;
	struct sx150x_gpio *sx150x = context;
	struct i2c_client *i2c = sx150x->client;
	const int width = sx150x_regmap_reg_width(sx150x, reg);
	unsigned int idx, val;

	/*
	 * There are four potential cases coverd by this function:
	 *
	 * 1) 8-pin chip, single configuration bit register
	 *
	 *	This is trivial the code below just needs to read:
	 *		reg  [ 7 6 5 4 3 2 1 0 ]
	 *
	 * 2) 16-pin chip, single configuration bit register
	 *
	 *	The read will be done as follows:
	 *		reg     [ f e d c b a 9 8 ]
	 *		reg + 1 [ 7 6 5 4 3 2 1 0 ]
	 *
	 */

	for (n = width, val = 0, idx = reg; n > 0; n -= 8, idx++) {
		val <<= 8;

		ret = i2c_smbus_read_byte_data(i2c, idx);
		if (ret < 0)
			return ret;

		val |= ret;
	}

	*result = val;

	return 0;
}

static int sx150x_regmap_reg_write(void *context, unsigned int reg,
				   unsigned int val)
{
	int ret, n;
	struct sx150x_gpio *sx150x = context;
	struct i2c_client *i2c = sx150x->client;
	const int width = sx150x_regmap_reg_width(sx150x, reg);

	n = width - 8;
	do {
		const u8 byte = (val >> n) & 0xff;

		ret = i2c_smbus_write_byte_data(i2c, reg, byte);
		if (ret < 0)
			return ret;

		reg++;
		n -= 8;
	} while (n >= 0);

	return 0;
}

static const struct regmap_config sx150x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,

	.max_register = SX150X_MAX_REGISTER,
};

static const struct regmap_bus sx150x_regmap_bus = {
	.reg_read  = sx150x_regmap_reg_read,
	.reg_write = sx150x_regmap_reg_write,
};

static struct gpio_ops sx150x_gpio_ops = {
	.direction_input   = sx150x_gpio_direction_input,
	.direction_output  = sx150x_gpio_direction_output,
	.get_direction	   = sx150x_gpio_get_direction,
	.get		   = sx150x_gpio_get,
	.set		   = sx150x_gpio_set,
};

static int sx150x_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sx150x_gpio *sx150x;
	const struct sx150x_device_data *data;

	data = of_device_get_match_data(dev);
	if (!data)
		return -EINVAL;

	sx150x = xzalloc(sizeof(*sx150x));

	sx150x->regmap = regmap_init(dev, &sx150x_regmap_bus,
				     sx150x, &sx150x_regmap_config);
	sx150x->client = client;
	sx150x->data = data;
	sx150x->gpio.ops   = &sx150x_gpio_ops;
	sx150x->gpio.base  = -1;
	sx150x->gpio.ngpio = sx150x->data->ngpios;
	sx150x->gpio.dev   = &client->dev;

	return gpiochip_add(&sx150x->gpio);
}

static __maybe_unused struct of_device_id sx150x_dt_ids[] = {
	{ .compatible = "semtech,sx1503q", .data = &sx1503q_device_data, },
	{ }
};

static struct driver_d sx150x_driver = {
	.name = "sx150x",
	.probe = sx150x_probe,
	.of_compatible = sx150x_dt_ids,
};
device_i2c_driver(sx150x_driver);
