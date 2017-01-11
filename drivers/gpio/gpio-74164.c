/*
 * Generic serial-in/parallel-out 8-bits shift register GPIO driver
 *   e.g. for 74x164
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux driver
 *  Copyright (C) 2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2010 Miguel Gaio <miguel.gaio@efixo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <spi/spi.h>

#define MAX_REGS	4

struct gpio_74164 {
	struct gpio_chip chip;
	struct spi_device *spi;
	u8 buffer[MAX_REGS];
	u8 num_regs;
};

#define gc_to_gpio_74164(c)	container_of(c, struct gpio_74164, chip)

/*
 * Since the registers are chained, every byte sent will make
 * the previous byte shift to the next register in the
 * chain. Thus, the first byte send will end up in the last
 * register at the end of the transfer. So, to have a logical
 * numbering, send the bytes in reverse order so that the last
 * byte of the buffer will end up in the last register.
 */
static int gpio_74164_update_buffers(struct gpio_74164 *priv)
{
	u8 b[MAX_REGS];
	int n;

	for (n = 0; n < priv->num_regs; n++)
		b[priv->num_regs - n - 1] = priv->buffer[n];
	spi_write(priv->spi, b, priv->num_regs);

	return 0;
}

static int gpio_74164_get_value(struct gpio_chip *chip, unsigned off)
{
	struct gpio_74164 *priv = gc_to_gpio_74164(chip);
	u8 bank = off / 8;
	u8 pin = off % 8;

	return (priv->buffer[bank] >> pin) & 1;
}

static void gpio_74164_set_value(struct gpio_chip *chip,
				 unsigned off, int val)
{
	struct gpio_74164 *priv = gc_to_gpio_74164(chip);
	u8 bank = off / 8;
	u8 pin = off % 8;

	if (val)
		priv->buffer[bank] |= BIT(pin);
	else
		priv->buffer[bank] &= ~BIT(pin);

	gpio_74164_update_buffers(priv);
}

static int gpio_74164_direction_output(struct gpio_chip *chip,
				       unsigned off, int val)
{
	gpio_74164_set_value(chip, off, val);
	return 0;
}

static struct gpio_ops gpio_74164_ops = {
	.direction_output = gpio_74164_direction_output,
	.get = gpio_74164_get_value,
	.set = gpio_74164_set_value,
};

static struct platform_device_id gpio_74164_ids[] = {
	{ "74hc164" },
	{ "74lvc594" },
	{ "74hc595" },
	{ }
};

static int gpio_74164_probe(struct device_d *dev)
{
	struct spi_device *spi = (struct spi_device *)dev->type_data;
	struct gpio_74164 *priv;
	u32 num_regs = 1;

	dev->id = DEVICE_ID_DYNAMIC;
	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->device_node) {
		dev->id = of_alias_get_id(dev->device_node, "gpio");
		of_property_read_u32(dev->device_node, "registers-number",
				     &num_regs);
	}

	if (num_regs > MAX_REGS)
		num_regs = MAX_REGS;

	priv = xzalloc(sizeof(*priv));
	priv->spi = spi;
	priv->num_regs = num_regs;
	priv->chip.dev = dev;
	priv->chip.ops = &gpio_74164_ops;
	priv->chip.base = dev->id * 32;
	priv->chip.ngpio = num_regs * 8;

	return gpiochip_add(&priv->chip);
}

static struct driver_d gpio_74164_driver = {
	.name = "gpio-74164",
	.probe = gpio_74164_probe,
	.id_table = gpio_74164_ids,
};
device_spi_driver(gpio_74164_driver);
