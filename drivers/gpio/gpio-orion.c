/*
 * Marvell Orion/MVEBU SoC GPIO driver
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>

struct orion_gpio_regs {
	u32 data_o;
	u32 data_o_en;
	u32 blink;
	u32 data_i_pol;
	u32 data_i;
	u32 irq_cause;
	u32 irq_mask;
	u32 irq_level_mask;
};

struct orion_gpio_chip {
	struct gpio_chip chip;
	struct orion_gpio_regs __iomem *regs;
};

static int orion_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	struct orion_gpio_chip *gpio =
		container_of(chip, struct orion_gpio_chip, chip);
	writel(readl(&gpio->regs->data_o_en) | BIT(off),
		&gpio->regs->data_o_en);
	return 0;
}

static int orion_gpio_direction_output(
	struct gpio_chip *chip, unsigned off, int value)
{
	struct orion_gpio_chip *gpio =
		container_of(chip, struct orion_gpio_chip, chip);
	gpio->chip.ops->set(chip, off, value);
	writel(readl(&gpio->regs->data_o_en) & ~BIT(off),
		&gpio->regs->data_o_en);
	return 0;
}

static int orion_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct orion_gpio_chip *gpio =
		container_of(chip, struct orion_gpio_chip, chip);
	return (readl(&gpio->regs->data_i) & BIT(off)) ? 1 : 0;
}

static void orion_gpio_set_value(
	struct gpio_chip *chip, unsigned off, int value)
{
	struct orion_gpio_chip *gpio =
		container_of(chip, struct orion_gpio_chip, chip);
	u32 val;

	val = readl(&gpio->regs->data_o);
	if (value)
		val |= BIT(off);
	else
		val &= ~BIT(off);
	writel(val, &gpio->regs->data_o);
}

static struct gpio_ops orion_gpio_ops = {
	.direction_input = orion_gpio_direction_input,
	.direction_output = orion_gpio_direction_output,
	.get = orion_gpio_get_value,
	.set = orion_gpio_set_value,
};

static int orion_gpio_probe(struct device_d *dev)
{
	struct orion_gpio_chip *gpio;

	dev->id = of_alias_get_id(dev->device_node, "gpio");
	if (dev->id < 0)
		return dev->id;

	gpio = xzalloc(sizeof(*gpio));
	gpio->regs = dev_request_mem_region(dev, 0);
	if (!gpio->regs) {
		free(gpio);
		return -EINVAL;
	}
	gpio->chip.dev = dev;
	gpio->chip.ops = &orion_gpio_ops;
	gpio->chip.base = dev->id * 32;
	gpio->chip.ngpio = 32;
	of_property_read_u32(dev->device_node, "ngpios", &gpio->chip.ngpio);

	gpiochip_add(&gpio->chip);

	dev_dbg(dev, "probed gpio%d with base %d\n", dev->id, gpio->chip.base);

	return 0;
}

static struct of_device_id orion_gpio_dt_ids[] = {
	{ .compatible = "marvell,orion-gpio", },
	{ }
};

static struct driver_d orion_gpio_driver = {
	.name = "orion-gpio",
	.probe = orion_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(orion_gpio_dt_ids),
};

static int orion_gpio_init(void)
{
	return platform_driver_register(&orion_gpio_driver);
}
postcore_initcall(orion_gpio_init);
