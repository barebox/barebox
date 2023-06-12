// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 David Brownell

/*
 * Driver for pcf857x, pca857x, and pca967x I2C GPIO expanders
 *
 * This code was ported from linux-5.1 kernel by Michael Grzeschik.
 */

#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <gpio.h>

static const struct platform_device_id pcf857x_id[] = {
	{ "pcf8574", 8 },
	{ "pcf8574a", 8 },
	{ "pca8574", 8 },
	{ "pca9670", 8 },
	{ "pca9672", 8 },
	{ "pca9674", 8 },
	{ "pcf8575", 16 },
	{ "pca8575", 16 },
	{ "pca9671", 16 },
	{ "pca9673", 16 },
	{ "pca9675", 16 },
	{ "max7328", 8 },
	{ "max7329", 8 },
	{ }
};

/*
 * The pcf857x, pca857x, and pca967x chips only expose one read and one
 * write register.  Writing a "one" bit (to match the reset state) lets
 * that pin be used as an input; it's not an open-drain model, but acts
 * a bit like one.  This is described as "quasi-bidirectional"; read the
 * chip documentation for details.
 *
 * Many other I2C GPIO expander chips (like the pca953x models) have
 * more complex register models and more conventional circuitry using
 * push/pull drivers.  They often use the same 0x20..0x27 addresses as
 * pcf857x parts, making the "legacy" I2C driver model problematic.
 */
struct pcf857x {
	struct gpio_chip	chip;
	struct i2c_client	*client;
	unsigned		out;		/* software latch */

	int (*write)(struct i2c_client *client, unsigned data);
	int (*read)(struct i2c_client *client);
};

static inline struct pcf857x *to_pcf(struct gpio_chip *gc)
{
	return container_of(gc, struct pcf857x, chip);
}

/*-------------------------------------------------------------------------*/

/* Talk to 8-bit I/O expander */

static int i2c_write_le8(struct i2c_client *client, unsigned data)
{
	return i2c_smbus_write_byte(client, data);
}

static int i2c_read_le8(struct i2c_client *client)
{
	return (int)i2c_smbus_read_byte(client);
}

/* Talk to 16-bit I/O expander */

static int i2c_write_le16(struct i2c_client *client, unsigned word)
{
	u8 buf[2] = { word & 0xff, word >> 8, };
	int ret;

	ret = i2c_master_send(client, buf, 2);
	return (ret < 0) ? ret : 0;
}

static int i2c_read_le16(struct i2c_client *client)
{
	u8 buf[2];
	int ret;

	ret = i2c_master_recv(client, buf, 2);
	if (ret < 0)
		return ret;
	return (buf[1] << 8) | buf[0];
}

/*-------------------------------------------------------------------------*/

static int pcf857x_input(struct gpio_chip *chip, unsigned offset)
{
	struct pcf857x	*gpio = to_pcf(chip);
	int		ret;

	gpio->out |= (1 << offset);
	ret = gpio->write(gpio->client, gpio->out);

	return ret;
}

static int pcf857x_get(struct gpio_chip *chip, unsigned offset)
{
	struct pcf857x	*gpio = to_pcf(chip);
	int		value;

	value = gpio->read(gpio->client);
	return (value < 0) ? value : !!(value & (1 << offset));
}

static int pcf857x_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct pcf857x	*gpio = to_pcf(chip);
	unsigned	bit = 1 << offset;
	int		ret;

	if (value)
		gpio->out |= bit;
	else
		gpio->out &= ~bit;
	ret = gpio->write(gpio->client, gpio->out);

	return ret;
}

static void pcf857x_set(struct gpio_chip *chip, unsigned offset, int value)
{
	pcf857x_output(chip, offset, value);
}

/*-------------------------------------------------------------------------*/

static struct gpio_ops pcf857x_gpio_ops = {
	.direction_input  = pcf857x_input,
	.direction_output = pcf857x_output,
	.get = pcf857x_get,
	.set = pcf857x_set,
};

static int pcf857x_probe(struct device *dev)
{
	struct i2c_client		*client = to_i2c_client(dev);
	struct device_node		*np = dev->of_node;
	struct pcf857x			*gpio;
	unsigned long			driver_data;
	unsigned int			n_latch = 0;
	int				ret;

	if (!np)
		return -EINVAL;

	of_property_read_u32(np, "lines-initial-states", &n_latch);

	/* Allocate, initialize, and register this gpio_chip. */
	gpio = xzalloc(sizeof(*gpio));

	ret = dev_get_drvdata(dev, (const void **)&driver_data);
	if (ret)
		return ret;

	gpio->chip.base			= -1;
	gpio->chip.ops			= &pcf857x_gpio_ops;
	gpio->chip.ngpio		= driver_data;
	gpio->chip.dev			= &client->dev;

	/* NOTE:  the OnSemi jlc1562b is also largely compatible with
	 * these parts, notably for output.  It has a low-resolution
	 * DAC instead of pin change IRQs; and its inputs can be the
	 * result of comparators.
	 */

	/* 8574 addresses are 0x20..0x27; 8574a uses 0x38..0x3f;
	 * 9670, 9672, 9764, and 9764a use quite a variety.
	 *
	 * NOTE: we don't distinguish here between *4 and *4a parts.
	 */
	switch (gpio->chip.ngpio) {
	case 8:
		gpio->write	= i2c_write_le8;
		gpio->read	= i2c_read_le8;
		break;
	/* '75/'75c addresses are 0x20..0x27, just like the '74;
	 * the '75c doesn't have a current source pulling high.
	 * 9671, 9673, and 9765 use quite a variety of addresses.
	 *
	 * NOTE: we don't distinguish here between '75 and '75c parts.
	 */
	case 16:
		gpio->write	= i2c_write_le16;
		gpio->read	= i2c_read_le16;
		break;
	default:
		dev_warn(&client->dev, "unsupported number of gpios\n");
		return -EINVAL;
	}

	gpio->client = client;

	/* NOTE:  these chips have strange "quasi-bidirectional" I/O pins.
	 * We can't actually know whether a pin is configured (a) as output
	 * and driving the signal low, or (b) as input and reporting a low
	 * value ... without knowing the last value written since the chip
	 * came out of reset (if any).  We can't read the latched output.
	 *
	 * In short, the only reliable solution for setting up pin direction
	 * is to do it explicitly.
	 *
	 * Using n_latch avoids that trouble.  When left initialized to zero,
	 * our software copy of the "latch" then matches the chip's all-ones
	 * reset state.  Otherwise it flags pins to be driven low.
	 */
	gpio->out = ~n_latch;

	return gpiochip_add(&gpio->chip);
}

static const struct of_device_id pcf857x_dt_ids[] = {
	{ .compatible = "nxp,pcf8574", .data = (void *)8 },
	{ .compatible = "nxp,pcf8574a", .data = (void *)8 },
	{ .compatible = "nxp,pca8574", .data = (void *)8 },
	{ .compatible = "nxp,pca9670", .data = (void *)8 },
	{ .compatible = "nxp,pca9672", .data = (void *)8 },
	{ .compatible = "nxp,pca9674", .data = (void *)8 },
	{ .compatible = "nxp,pcf8575", .data = (void *)16 },
	{ .compatible = "nxp,pca8575", .data = (void *)16 },
	{ .compatible = "nxp,pca9671", .data = (void *)16 },
	{ .compatible = "nxp,pca9673", .data = (void *)16 },
	{ .compatible = "nxp,pca9675", .data = (void *)16 },
	{ .compatible = "maxim,max7328", .data = (void *)8 },
	{ .compatible = "maxim,max7329", .data = (void *)8 },
	{ }
};
MODULE_DEVICE_TABLE(of, pcf857x_dt_ids);

static struct driver pcf857x_driver = {
	.name	= "pcf857x",
	.probe	= pcf857x_probe,
	.of_compatible = DRV_OF_COMPAT(pcf857x_dt_ids),
	.id_table = pcf857x_id,
};
device_i2c_driver(pcf857x_driver);
