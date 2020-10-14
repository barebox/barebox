/*
 * Copyright (C) 2012 Pengutronix
 * Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <init.h>
#include <mfd/stmpe-i2c.h>

#define GPIO_BASE	0x80
#define GPIO_SET	(GPIO_BASE + 0x02)
#define GPIO_CLR	(GPIO_BASE + 0x04)
#define GPIO_MP		(GPIO_BASE + 0x06)
#define GPIO_SET_DIR	(GPIO_BASE + 0x08)
#define GPIO_ED		(GPIO_BASE + 0x0a)
#define GPIO_RE		(GPIO_BASE + 0x0c)
#define GPIO_FE		(GPIO_BASE + 0x0e)
#define GPIO_PULL_UP	(GPIO_BASE + 0x10)
#define GPIO_AF		(GPIO_BASE + 0x12)
#define GPIO_LT		(GPIO_BASE + 0x16)

#define OFFSET(gpio)	(0xff & (1 << (gpio)) ? 1 : 0)

struct stmpe_gpio_chip {
	struct gpio_chip chip;
	struct stmpe_client_info *ci;
};

static void stmpe_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct stmpe_gpio_chip *stmpegpio = container_of(chip, struct stmpe_gpio_chip, chip);
	struct stmpe_client_info *ci = (struct stmpe_client_info *)stmpegpio->ci;
	int ret;
	u8 val;

	ci->read_reg(ci->stmpe, GPIO_MP + OFFSET(gpio), &val);

	val |= 1 << (gpio % 8);

	if (value)
		ret = ci->write_reg(ci->stmpe, GPIO_SET + OFFSET(gpio), val);
	else
		ret = ci->write_reg(ci->stmpe, GPIO_CLR + OFFSET(gpio), val);

	if (ret)
		dev_err(chip->dev, "write failed!\n");
}

static int stmpe_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct stmpe_gpio_chip *stmpegpio = container_of(chip, struct stmpe_gpio_chip, chip);
	struct stmpe_client_info *ci = (struct stmpe_client_info *)stmpegpio->ci;
	int ret;
	u8 val;

	ci->read_reg(ci->stmpe, GPIO_SET_DIR + OFFSET(gpio), &val);
	val &= ~(1 << (gpio % 8));
	ret = ci->write_reg(ci->stmpe, GPIO_SET_DIR + OFFSET(gpio), val);

	if (ret)
		dev_err(chip->dev, "couldn't change direction. Write failed!\n");

	return ret;
}

static int stmpe_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct stmpe_gpio_chip *stmpegpio = container_of(chip, struct stmpe_gpio_chip, chip);
	struct stmpe_client_info *ci = (struct stmpe_client_info *)stmpegpio->ci;
	int ret;
	u8 val;

	ci->read_reg(ci->stmpe, GPIO_SET_DIR + OFFSET(gpio), &val);
	val |= 1 << (gpio % 8);
	ret = ci->write_reg(ci->stmpe, GPIO_SET_DIR + OFFSET(gpio), val);

	stmpe_gpio_set_value(chip, gpio, value);

	if (ret)
		dev_err(chip->dev, "couldn't change direction. Write failed!\n");

	return ret;
}

static int stmpe_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct stmpe_gpio_chip *stmpegpio = container_of(chip, struct stmpe_gpio_chip, chip);
	struct stmpe_client_info *ci = (struct stmpe_client_info *)stmpegpio->ci;
	u8 val;
	int ret;

	ret = ci->read_reg(ci->stmpe, GPIO_MP + OFFSET(gpio), &val);

	if (ret)
		dev_err(chip->dev, "read failed\n");

	return val & (1 << (gpio % 8)) ? 1 : 0;
}

static struct gpio_ops stmpe_gpio_ops = {
	.direction_input = stmpe_gpio_direction_input,
	.direction_output = stmpe_gpio_direction_output,
	.get = stmpe_gpio_get_value,
	.set = stmpe_gpio_set_value,
};

static int stmpe_gpio_probe(struct device_d *dev)
{
	struct stmpe_gpio_chip *stmpegpio;
	struct stmpe_client_info *ci;
	int ret;

	stmpegpio = xzalloc(sizeof(*stmpegpio));

	stmpegpio->chip.ops = &stmpe_gpio_ops;
	stmpegpio->ci = dev->platform_data;

	ci = (struct stmpe_client_info *)stmpegpio->ci;

	if (ci->stmpe->pdata->gpio_base)
		stmpegpio->chip.base = ci->stmpe->pdata->gpio_base;
	else
		stmpegpio->chip.base = -1;
	stmpegpio->chip.ngpio = 16;
	stmpegpio->chip.dev = dev;

	ret = gpiochip_add(&stmpegpio->chip);

	if (ret) {
		dev_err(dev, "couldn't add gpiochip\n");
		return ret;
	}

	dev_dbg(dev, "probed stmpe gpiochip%d with base %d\n", dev->id, stmpegpio->chip.base);
	return 0;
}

static struct driver_d stmpe_gpio_driver = {
	.name = "stmpe-gpio",
	.probe = stmpe_gpio_probe,
};

coredevice_platform_driver(stmpe_gpio_driver);
