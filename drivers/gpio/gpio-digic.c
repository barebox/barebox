// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Antony Pavlov <antonynpavlov@gmail.com>

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <init.h>

/*
 * See http://magiclantern.wikia.com/wiki/Register_Map#GPIO_Ports
 */

#define DIGIC_GPIO_IN_LVL	1
#define DIGIC_GPIO_OUT_LVL	2
#define DIGIC_GPIO_DIR	4
#define DIGIC_GPIO_TRISTATE	8

struct digic_gpio_chip {
	void __iomem		*base;
	struct gpio_chip	gc;
};

#define to_digic_gpio_chip(c) container_of(c, struct digic_gpio_chip, gc)

static inline uint32_t digic_gpio_readl(struct digic_gpio_chip *chip,
					uint32_t offset)
{
	return readl(chip->base + 4 * offset);
}

static inline void digic_gpio_writel(struct digic_gpio_chip *chip,
					uint32_t value, uint32_t offset)
{
	writel(value, chip->base + 4 * offset);
}

static int digic_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct digic_gpio_chip *chip = to_digic_gpio_chip(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;

	return digic_gpio_readl(chip, offset) & DIGIC_GPIO_IN_LVL;
}

static void digic_gpio_set_value(struct gpio_chip *gc, unsigned offset,
					int value)
{
	struct digic_gpio_chip *chip = to_digic_gpio_chip(gc);
	uint32_t t;

	if (offset >= gc->ngpio)
		return;

	t = digic_gpio_readl(chip, offset);
	/* Port direction (1 = OUT, 0 = IN) */
	if (value)
		t |= DIGIC_GPIO_OUT_LVL;
	else
		t &= ~(DIGIC_GPIO_OUT_LVL);
	digic_gpio_writel(chip, t, offset);
}

static int digic_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct digic_gpio_chip *chip = to_digic_gpio_chip(gc);
	uint32_t t;

	if (offset >= gc->ngpio)
		return -EINVAL;

	t = digic_gpio_readl(chip, offset);
	/* Port direction (1 = OUT, 0 = IN) */
	t &= ~(DIGIC_GPIO_DIR);
	digic_gpio_writel(chip, t, offset);

	return 0;
}

static int digic_gpio_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct digic_gpio_chip *chip = to_digic_gpio_chip(gc);
	uint32_t t;

	if (offset >= gc->ngpio)
		return -EINVAL;

	t = digic_gpio_readl(chip, offset);
	/* Port direction (1 = OUT, 0 = IN) */
	t |= DIGIC_GPIO_DIR;
	digic_gpio_writel(chip, t, offset);

	digic_gpio_set_value(gc, offset, value);

	return 0;
}

static struct gpio_ops digic_gpio_ops = {
	.direction_input = digic_gpio_direction_input,
	.direction_output = digic_gpio_direction_output,
	.get = digic_gpio_get_value,
	.set = digic_gpio_set_value,
};

static int digic_gpio_probe(struct device *dev)
{
	struct resource *iores;
	struct digic_gpio_chip *chip;
	struct resource *res;
	resource_size_t rsize;
	int ret = -EINVAL;

	chip = xzalloc(sizeof(*chip));

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res)
		goto err;

	rsize = resource_size(res);
	chip->gc.ngpio = rsize / sizeof(int32_t);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	chip->base = IOMEM(iores->start);
	chip->gc.ops = &digic_gpio_ops;
	chip->gc.base = 0;

	chip->gc.dev = dev;

	ret = gpiochip_add(&chip->gc);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		goto err;
	}

	dev_info(dev, "probed gpiochip%d with base %d\n",
			dev->id, chip->gc.base);

	return 0;

err:
	kfree(chip);

	return ret;
}

static __maybe_unused struct of_device_id digic_gpio_dt_ids[] = {
	{
		.compatible = "canon,digic-gpio",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, digic_gpio_dt_ids);

static struct driver digic_gpio_driver = {
	.name = "digic-gpio",
	.probe = digic_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(digic_gpio_dt_ids),
};

coredevice_platform_driver(digic_gpio_driver);
