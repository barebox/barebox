/*
 * Copyright (C) 2015 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <clock.h>
#include <driver.h>
#include <gpio.h>
#include <restart.h>
#include <i2c/i2c.h>
#include <linux/bitops.h>
#include <malloc.h>
#include <notifier.h>
#include <reset_source.h>
#include <watchdog.h>

struct da9063 {
	struct restart_handler	restart;
	struct watchdog		wd;
	struct gpio_chip	gpio;
	struct i2c_client	*client;
	/* dummy client for accessing bank #1 */
	struct i2c_client	*client1;
	struct device_d		*dev;
	unsigned int		timeout;
	uint64_t		last_ping;
};

/* forbidden/impossible value; timeout will be set to this value initially to
 * detect ping vs. set_timeout() operations. */
#define DA9063_INITIAL_TIMEOUT	(~0u)

/* System Control and Event Registers */
#define DA9063_REG_FAULT_LOG	0x05
#define DA9063_REG_CONTROL_D	0x11
#define DA9063_REG_CONTROL_F	0x13

/* bank1: control register I */
#define DA9063_REG1_CONFIG_I	0x10e

#define DA9062AA_DEVICE_ID	0x181

/* DA9063_REG_FAULT_LOG (addr=0x05) */
#define DA9063_TWD_ERROR	0x01
#define DA9063_POR		0x02
#define DA9063_NSHUTDOWN	0x40

/* DA9063_REG_CONTROL_D (addr=0x11) */
#define DA9063_TWDSCALE_MASK	0x07

/* DA9063_REG_CONTROL_F (addr=0x13) */
#define DA9063_WATCHDOG		0x01
#define DA9063_SHUTDOWN		0x02

/* DA9063_REG_CONTROL_I (addr=0x10e) */
#define DA9062_WATCHDOG_SD	BIT(3)

#define DA9062AA_STATUS_B       0x002
#define DA9062AA_GPIO_0_1       0x015
#define DA9062AA_GPIO_MODE0_4	0x01D

/* DA9062AA_GPIO_0_1 (addr=0x015) */
#define DA9062AA_GPIO0_PIN_MASK         0x03

#define DA9062_PIN_SHIFT(offset)	(4 * (offset % 2))
#define DA9062_PIN_ALTERNATE		0x00 /* gpio alternate mode */
#define DA9062_PIN_GPI			0x01 /* gpio in */
#define DA9062_PIN_GPO_OD		0x02 /* gpio out open-drain */
#define DA9062_PIN_GPO_PP		0x03 /* gpio out push-pull */

struct da906x_device_data {
	int	(*init)(struct da9063 *priv);
};

static int da906x_reg_update(struct da9063 *priv, unsigned int reg,
			     uint8_t mask, uint8_t val)
{
	struct i2c_client *client;
	uint8_t tmp;
	int ret;

	if (reg < 0x100)
		client = priv->client;
	else if (reg < 0x200)
		client = priv->client1;
	else
		/* this should/can not happen because function is usually
		 * called with a static register number; warn about it
		 * below */
		client = NULL;

	if (WARN_ON(!client))
		return -EINVAL;

	ret = i2c_read_reg(client, reg & 0xffu, &tmp, 1);
	if (ret < 0) {
		dev_warn(priv->dev, "failed to read reg %02x\n", reg);
		return ret;
	}

	tmp &= ~mask;
	tmp |= val;

	ret = i2c_write_reg(client, reg & 0xffu, &tmp, 1);
	if (ret < 0) {
		dev_warn(priv->dev, "failed to write %02x into reg %02x\n",
			 tmp, reg);
		return ret;
	}

	return 0;
}

static inline struct da9063 *to_da9063(struct gpio_chip *chip)
{
	return container_of(chip, struct da9063, gpio);
}

static int da9063_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct da9063 *priv = to_da9063(chip);
	u8 mask, mode;

	mode = DA9062_PIN_GPI << DA9062_PIN_SHIFT(offset);
	mask = DA9062AA_GPIO0_PIN_MASK << DA9062_PIN_SHIFT(offset);

	return da906x_reg_update(priv, DA9062AA_GPIO_0_1 + (offset >> 1),
				 mask, mode);
}

static int da9063_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
					int value)
{
	struct da9063 *priv = to_da9063(chip);

	return da906x_reg_update(priv, DA9062AA_GPIO_MODE0_4, BIT(offset),
				 value << offset);
}

static int da9063_gpio_get_pin_mode(struct da9063 *priv, unsigned offset)
{
	int ret;
	u8 val;

	ret = i2c_read_reg(priv->client, DA9062AA_GPIO_0_1 + (offset >> 1),
			   &val, 1);
	if (ret < 0)
		return ret;

	val >>= DA9062_PIN_SHIFT(offset);
	val &= DA9062AA_GPIO0_PIN_MASK;

	return val;
}

static int da9063_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct da9063 *priv = to_da9063(chip);
	int gpio_dir;
	int ret;
	u8 val;

	gpio_dir = da9063_gpio_get_pin_mode(priv, offset);
	if (gpio_dir < 0)
		return gpio_dir;

	switch (gpio_dir) {
	case DA9062_PIN_ALTERNATE:
		return -ENOTSUPP;
	case DA9062_PIN_GPI:
		ret = i2c_read_reg(priv->client, DA9062AA_STATUS_B, &val, 1);
		if (ret < 0)
			return ret;
		break;
	case DA9062_PIN_GPO_OD:
		/* falltrough */
	case DA9062_PIN_GPO_PP:
		ret = i2c_read_reg(priv->client, DA9062AA_GPIO_MODE0_4, &val, 1);
		if (ret < 0)
			return ret;
	}

	return val & BIT(offset);
}

static int da9063_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct da9063 *priv = to_da9063(chip);
	int gpio_dir;

	gpio_dir = da9063_gpio_get_pin_mode(priv, offset);
	if (gpio_dir < 0)
		return gpio_dir;

	switch (gpio_dir) {
	case DA9062_PIN_ALTERNATE:
		return -ENOTSUPP;
	case DA9062_PIN_GPI:
		return 1;
	case DA9062_PIN_GPO_OD:
		/* falltrough */
	case DA9062_PIN_GPO_PP:
		return 0;
	}

	return -EINVAL;
}

static void da9063_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct da9063 *priv = to_da9063(chip);

	da906x_reg_update(priv, DA9062AA_GPIO_MODE0_4, BIT(offset),
			  value << offset);

}

static struct gpio_ops da9063_gpio_ops = {
	.direction_input	= da9063_gpio_direction_input,
	.direction_output	= da9063_gpio_direction_output,
	.get_direction		= da9063_gpio_get_direction,
	.get			= da9063_gpio_get,
	.set			= da9063_gpio_set,
};

static int da9063_watchdog_ping(struct da9063 *priv)
{
	int ret;
	u8 val;

	/*
	 * The watchdog has a cool down phase of 200ms and if we ping to fast
	 * the da9062/3 resets the system. Reject those requests has a maximum
	 * failure of 10% if the watchdog timeout is set to 2.048s.
	 */
	if (!is_timeout(priv->last_ping, 200 * MSECOND))
		return 0;

	dev_dbg(priv->dev, "ping\n");

	/* reset watchdog timer; register is self clearing */
	val = DA9063_WATCHDOG;
	ret = i2c_write_reg(priv->client, DA9063_REG_CONTROL_F, &val, 1);
	if (ret < 0)
		return ret;

	priv->last_ping = get_time_ns();

	return 0;
}

static int da9063_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct da9063 *priv = container_of(wd, struct da9063, wd);
	struct device_d *dev = priv->dev;
	unsigned int scale = 0;
	int ret;

	if (timeout > 131)
		return -EINVAL;

	timeout *= 1000; /* convert to ms */

	if (timeout == priv->timeout)
		/* set_timeout called with previous parameter; just ping the
		 * watchdog */
		goto out;

	if (timeout) {
		scale = 0;
		while (timeout > (2048 << scale) && scale <= 6)
			scale++;
		dev_dbg(dev, "calculated TWDSCALE=%u (req=%ims calc=%ims)\n",
			scale + 1, timeout, 2048 << scale);
		scale++; /* scale 0 disables the WD */
	}

	ret = da906x_reg_update(priv, DA9063_REG_CONTROL_D,
				DA9063_TWDSCALE_MASK, scale);
	if (ret < 0)
		return ret;

	priv->timeout = timeout;

out:
	return da9063_watchdog_ping(priv);
}

static void da9063_detect_reset_source(struct da9063 *priv)
{
	int ret;
	u8 val;
	enum reset_src_type type;

	ret = i2c_read_reg(priv->client, DA9063_REG_FAULT_LOG, &val, 1);
	if (ret < 0)
		return;

	/* Write one to clear */
	i2c_write_reg(priv->client, DA9063_REG_FAULT_LOG, &val, 1);

	if (val & DA9063_TWD_ERROR)
		type = RESET_WDG;
	else if (val & DA9063_POR)
		type = RESET_POR;
	else if (val & DA9063_NSHUTDOWN)
		type = RESET_RST;
	else
		return;

	reset_source_set_device(priv->dev, type);
}

static void da9063_restart(struct restart_handler *rst)
{
	struct da9063 *priv = container_of(rst, struct da9063, restart);

	u8 val = DA9063_SHUTDOWN;

	i2c_write_reg(priv->client, DA9063_REG_CONTROL_F, &val, 1);
}

static int da9062_device_init(struct da9063 *priv)
{
	int ret;
	uint8_t id[4];

	priv->client1 = i2c_new_dummy(priv->client->adapter,
				      priv->client->addr + 1);
	if (!priv->client1) {
		dev_warn(priv->dev, "failed to create bank 1 device\n");
		/* TODO: return -EINVAL; i2c api does not return more
		 * details */
		return -EINVAL;
	}

	ret = i2c_read_reg(priv->client1, DA9062AA_DEVICE_ID & 0xffu,
			   id, sizeof id);
	if (ret < 0) {
		dev_warn(priv->dev, "failed to read ID: %d\n", ret);
		return ret;
	}

	dev_info(priv->dev, "da9062 with id %02x.%02x.%02x.%02x detected\n",
		 id[0], id[1], id[2], id[3]);

	/* clear CONFIG_I[WATCHDOG_SD] */
	ret = da906x_reg_update(priv, DA9063_REG1_CONFIG_I,
				DA9062_WATCHDOG_SD, DA9062_WATCHDOG_SD);

	return ret;
}

static struct da906x_device_data const	da9062_device_data = {
	.init	= da9062_device_init,
};

static int da9063_probe(struct device_d *dev)
{
	struct da9063 *priv = NULL;
	struct da906x_device_data const *dev_data;
	int ret;

	dev_data = device_get_match_data(dev);

	priv = xzalloc(sizeof(struct da9063));
	priv->wd.set_timeout = da9063_watchdog_set_timeout;
	priv->wd.hwdev = dev;
	priv->timeout = DA9063_INITIAL_TIMEOUT;
	priv->client = to_i2c_client(dev);
	priv->dev = dev;

	if (dev_data && dev_data->init) {
		ret = dev_data->init(priv);
		if (ret < 0)
			goto on_error;
	}

	ret = watchdog_register(&priv->wd);
	if (ret)
		goto on_error;

	da9063_detect_reset_source(priv);

	priv->restart.priority = of_get_restart_priority(dev->device_node);
	priv->restart.name = "da9063";
	priv->restart.restart = &da9063_restart;

	restart_handler_register(&priv->restart);

	if (IS_ENABLED(CONFIG_GPIOLIB)) {
		priv->gpio.base = -1;
		priv->gpio.ngpio = 5;
		priv->gpio.ops  = &da9063_gpio_ops;
		priv->gpio.dev = dev;
		ret = gpiochip_add(&priv->gpio);
		if (ret)
			goto on_error;
	}

	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->device_node)
		return of_platform_populate(dev->device_node, NULL, dev);

	return 0;

on_error:
	if (priv)
		free(priv);
	return ret;
}

static struct platform_device_id da9063_id[] = {
	{ "da9063", (uintptr_t)(NULL) },
	{ "da9062", (uintptr_t)(&da9062_device_data) },
	{ }
};

static struct of_device_id const	da906x_dt_ids[] = {
	{
		.compatible	= "dlg,da9063",
		.data		= NULL,
	}, {
		.compatible	= "dlg,da9062",
		.data		= &da9062_device_data,
	}, {
	}
};

static struct driver_d da9063_driver = {
	.name = "da9063",
	.probe = da9063_probe,
	.id_table = da9063_id,
	.of_compatible  = DRV_OF_COMPAT(da906x_dt_ids),
};
device_i2c_driver(da9063_driver);
