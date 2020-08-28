/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
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
#include <init.h>
#include <io.h>
#include <gpio.h>
#include <linux/err.h>
#include <malloc.h>

struct malta_i2c_gpio {
	void __iomem *base;
	struct gpio_chip chip;
};

#define MALTA_I2CINP 0
#define MALTA_I2COE  0x8
#define MALTA_I2COUT 0x10
#define MALTA_I2CSEL 0x18

static inline struct malta_i2c_gpio *chip_to_malta_i2c_gpio(struct gpio_chip *c)
{
	return container_of(c, struct malta_i2c_gpio, chip);
}

static inline void malta_i2c_gpio_write(struct malta_i2c_gpio *sc,
					u32 v, int reg)
{
	__raw_writel(v, sc->base + reg);
}

static inline u32 malta_i2c_gpio_read(struct malta_i2c_gpio *sc, int reg)
{
	return __raw_readl(sc->base + reg);
}

static inline int malta_i2c_gpio_get_bit(struct malta_i2c_gpio *sc,
						int reg, int bit)
{
	return !!(malta_i2c_gpio_read(sc, reg) & BIT(bit));
}

static inline void malta_i2c_gpio_set_bit(struct malta_i2c_gpio *sc,
						int reg, int bit, int v)
{
	u32 t;

	t = malta_i2c_gpio_read(sc, reg);
	if (v)
		t |= BIT(bit);
	else
		t &= ~BIT(bit);

	malta_i2c_gpio_write(sc, t, reg);
}

static int malta_i2c_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct malta_i2c_gpio *sc = chip_to_malta_i2c_gpio(chip);

	malta_i2c_gpio_set_bit(sc, MALTA_I2COE, gpio, 0);

	return 0;
}

static int malta_i2c_gpio_direction_output(struct gpio_chip *chip,
					unsigned gpio, int v)
{
	struct malta_i2c_gpio *sc = chip_to_malta_i2c_gpio(chip);

	malta_i2c_gpio_set_bit(sc, MALTA_I2COUT, gpio, v);
	malta_i2c_gpio_set_bit(sc, MALTA_I2COE, gpio, 1);

	return 0;
}

static int malta_i2c_gpio_get_direction(struct gpio_chip *chip, unsigned gpio)
{
	struct malta_i2c_gpio *sc = chip_to_malta_i2c_gpio(chip);

	if (malta_i2c_gpio_get_bit(sc, MALTA_I2COE, gpio))
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static int malta_i2c_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct malta_i2c_gpio *sc = chip_to_malta_i2c_gpio(chip);
	int v;

	v = malta_i2c_gpio_get_bit(sc, MALTA_I2CINP, gpio);

	pr_debug("%s: gpio_chip=%p gpio=%d value=%d\n",
			__func__, chip, gpio, v);

	return v;
}

static void malta_i2c_gpio_set_value(struct gpio_chip *chip,
					unsigned gpio, int v)
{
	struct malta_i2c_gpio *sc = chip_to_malta_i2c_gpio(chip);

	pr_debug("%s: gpio_chip=%p gpio=%d value=%d\n",
			__func__, chip, gpio, v);

	malta_i2c_gpio_set_bit(sc, MALTA_I2COUT, gpio, v);
}

static struct gpio_ops malta_i2c_gpio_ops = {
	.direction_input = malta_i2c_gpio_direction_input,
	.direction_output = malta_i2c_gpio_direction_output,
	.get_direction = malta_i2c_gpio_get_direction,
	.get = malta_i2c_gpio_get_value,
	.set = malta_i2c_gpio_set_value,
};

static int malta_i2c_gpio_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *gpio_base;
	struct malta_i2c_gpio *sc;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	gpio_base = IOMEM(iores->start);

	sc = xzalloc(sizeof(*sc));
	sc->base = gpio_base;
	sc->chip.ops = &malta_i2c_gpio_ops;
	sc->chip.base = -1;
	sc->chip.ngpio = 2;
	sc->chip.dev = dev;

	ret = gpiochip_add(&sc->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip\n");
		free(sc);
		return ret;
	}

	malta_i2c_gpio_write(sc, 1, MALTA_I2CSEL);

	dev_info(dev, "probed gpiochip%d with base %d\n",
		dev->id, sc->chip.base);

	return 0;
}

static __maybe_unused struct of_device_id malta_i2c_gpio_dt_ids[] = {
	{
		.compatible = "mti,malta-fpga-i2c-gpio",
	}, {
		/* sentinel */
	},
};

static struct driver_d malta_i2c_gpio_driver = {
	.name  = "malta-fpga-i2c-gpio",
	.probe = malta_i2c_gpio_probe,
	.of_compatible	= DRV_OF_COMPAT(malta_i2c_gpio_dt_ids),
};

coredevice_platform_driver(malta_i2c_gpio_driver);
