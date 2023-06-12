// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013,2014 Antony Pavlov <antonynpavlov@gmail.com>
// SPDX-FileCopyrightText: 2009-2010 Lars-Peter Clausen <lars@metafoo.de>

/*
 * Based on Linux JZ4740 platform GPIO support:
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <gpio.h>
#include <malloc.h>
#include <linux/err.h>

#define JZ_REG_GPIO_PIN			0x00
#define JZ_REG_GPIO_DATA		0x10
#define JZ_REG_GPIO_DATA_SET		0x14
#define JZ_REG_GPIO_DATA_CLEAR		0x18
#define JZ_REG_GPIO_DIRECTION		0x60
#define JZ_REG_GPIO_DIRECTION_SET	0x64
#define JZ_REG_GPIO_DIRECTION_CLEAR	0x68

#define GPIO_TO_BIT(gpio) BIT(gpio & 0x1f)
#define CHIP_TO_REG(chip, reg) (gpio_chip_to_jz4740_gpio_chip(chip)->base + (reg))

struct jz4740_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
};

static inline struct jz4740_gpio_chip *gpio_chip_to_jz4740_gpio_chip(struct gpio_chip *chip)
{
	return container_of(chip, struct jz4740_gpio_chip, chip);
}

static int jz4740_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	return !!(readl(CHIP_TO_REG(chip, JZ_REG_GPIO_PIN)) & BIT(gpio));
}

static void jz4740_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	uint32_t __iomem *reg = CHIP_TO_REG(chip, JZ_REG_GPIO_DATA_SET);
	reg += !value;
	writel(BIT(gpio), reg);
}

static int jz4740_gpio_get_direction(struct gpio_chip *chip, unsigned gpio)
{
	if (readl(CHIP_TO_REG(chip, JZ_REG_GPIO_DIRECTION)) & BIT(gpio))
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static int jz4740_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	writel(BIT(gpio), CHIP_TO_REG(chip, JZ_REG_GPIO_DIRECTION_CLEAR));

	return 0;
}

static int jz4740_gpio_direction_output(struct gpio_chip *chip, unsigned gpio,
	int value)
{
	jz4740_gpio_set_value(chip, gpio, value);
	writel(BIT(gpio), CHIP_TO_REG(chip, JZ_REG_GPIO_DIRECTION_SET));

	return 0;
}

static struct gpio_ops jz4740_gpio_ops = {
	.direction_input = jz4740_gpio_direction_input,
	.direction_output = jz4740_gpio_direction_output,
	.get_direction = jz4740_gpio_get_direction,
	.get = jz4740_gpio_get_value,
	.set = jz4740_gpio_set_value,
};

static int jz4740_gpio_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;
	struct jz4740_gpio_chip *jz4740_gpio;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	base = IOMEM(iores->start);

	jz4740_gpio = xzalloc(sizeof(*jz4740_gpio));
	jz4740_gpio->base = base;
	jz4740_gpio->chip.ops = &jz4740_gpio_ops;
	jz4740_gpio->chip.base = -1; /* dev->id * 32; */
	jz4740_gpio->chip.ngpio = 32;
	jz4740_gpio->chip.dev = dev;

	ret = gpiochip_add(&jz4740_gpio->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip\n");
		free(jz4740_gpio);
		return ret;
	}

	dev_dbg(dev, "probed gpiochip%d with base %d\n",
				dev->id, jz4740_gpio->chip.base);

	return 0;
}

static __maybe_unused struct of_device_id jz4740_gpio_dt_ids[] = {
	{
		.compatible = "ingenic,jz4740-gpio",
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, jz4740_gpio_dt_ids);

static struct driver jz4740_gpio_driver = {
	.name = "jz4740-gpio",
	.probe = jz4740_gpio_probe,
	.of_compatible	= DRV_OF_COMPAT(jz4740_gpio_dt_ids),
};

coredevice_platform_driver(jz4740_gpio_driver);
