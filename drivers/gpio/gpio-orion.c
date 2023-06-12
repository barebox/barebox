// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Marvell Orion/MVEBU SoC GPIO driver
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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

static int orion_gpio_probe(struct device *dev)
{
	struct resource *iores;
	struct orion_gpio_chip *gpio;
	int id;

	gpio = xzalloc(sizeof(*gpio));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		free(gpio);
		return PTR_ERR(iores);
	}
	gpio->regs = IOMEM(iores->start);
	gpio->chip.dev = dev;
	gpio->chip.ops = &orion_gpio_ops;

	id = of_alias_get_id(dev->of_node, "gpio");
	if (id < 0)
		return id;

	gpio->chip.base = id * 32;
	gpio->chip.ngpio = 32;
	of_property_read_u32(dev->of_node, "ngpios", &gpio->chip.ngpio);

	gpiochip_add(&gpio->chip);

	dev_dbg(dev, "probed gpio%d with base %d\n", dev->id, gpio->chip.base);

	return 0;
}

static struct of_device_id orion_gpio_dt_ids[] = {
	{ .compatible = "marvell,orion-gpio", },
	{ }
};
MODULE_DEVICE_TABLE(of, orion_gpio_dt_ids);

static struct driver orion_gpio_driver = {
	.name = "orion-gpio",
	.probe = orion_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(orion_gpio_dt_ids),
};
device_platform_driver(orion_gpio_driver);
