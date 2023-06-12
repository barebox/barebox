// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 SiFive
 */

#include <linux/basic_mmio_gpio.h>
#include <linux/printk.h>
#include <driver.h>
#include <errno.h>

#define SIFIVE_GPIO_INPUT_VAL	0x00
#define SIFIVE_GPIO_INPUT_EN	0x04
#define SIFIVE_GPIO_OUTPUT_EN	0x08
#define SIFIVE_GPIO_OUTPUT_VAL	0x0C
#define SIFIVE_GPIO_RISE_IE	0x18
#define SIFIVE_GPIO_FALL_IE	0x20
#define SIFIVE_GPIO_HIGH_IE	0x28
#define SIFIVE_GPIO_LOW_IE	0x30

#define SIFIVE_GPIO_MAX		32

static int __of_irq_count(struct device_node *np)
{
	unsigned npins = 0;

	of_get_property(np, "interrupts", &npins);

	return npins / sizeof(__be32);
}

static int sifive_gpio_probe(struct device *dev)
{
	struct bgpio_chip *bgc;
	struct resource *res;
	void __iomem *base;
	int ret, ngpio;

	bgc = xzalloc(sizeof(*bgc));

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res)) {
		dev_err(dev, "failed to request device memory\n");
		return PTR_ERR(res);
	}
	base = IOMEM(res->start);

	ngpio = __of_irq_count(dev->of_node);
	if (ngpio > SIFIVE_GPIO_MAX) {
		dev_err(dev, "Too many GPIO interrupts (max=%d)\n",
			SIFIVE_GPIO_MAX);
		return -ENXIO;
	}

	ret = bgpio_init(bgc, dev, 4,
			 base + SIFIVE_GPIO_INPUT_VAL,
			 base + SIFIVE_GPIO_OUTPUT_VAL,
			 NULL,
			 base + SIFIVE_GPIO_OUTPUT_EN,
			 base + SIFIVE_GPIO_INPUT_EN,
			 0);
	if (ret) {
		dev_err(dev, "unable to init generic GPIO\n");
		return ret;
	}

	/* Disable all GPIO interrupts */
	writel(0, base + SIFIVE_GPIO_RISE_IE);
	writel(0, base + SIFIVE_GPIO_FALL_IE);
	writel(0, base + SIFIVE_GPIO_HIGH_IE);
	writel(0, base + SIFIVE_GPIO_LOW_IE);

	bgc->gc.ngpio = ngpio;
	return gpiochip_add(&bgc->gc);
}

static const struct of_device_id sifive_gpio_match[] = {
	{ .compatible = "sifive,gpio0" },
	{ .compatible = "sifive,fu540-c000-gpio" },
	{ },
};
MODULE_DEVICE_TABLE(of, sifive_gpio_match);

static struct driver sifive_gpio_driver = {
	.name		= "sifive_gpio",
	.of_compatible	= sifive_gpio_match,
	.probe		= sifive_gpio_probe,
};
postcore_platform_driver(sifive_gpio_driver);
