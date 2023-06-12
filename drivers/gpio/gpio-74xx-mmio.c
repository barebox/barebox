// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * 74xx MMIO GPIO driver
 *
 *  Copyright (C) 2014 Alexander Shiyan <shc_work@mail.ru>
 *
 * Ported to barebox from linux-v5.4-rc6
 *   Copyright (C) 2019-2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of_device.h>

#include <linux/err.h>
#include <linux/basic_mmio_gpio.h>

#define MMIO_74XX_DIR_IN	(0 << 8)
#define MMIO_74XX_DIR_OUT	(1 << 8)
#define MMIO_74XX_BIT_CNT(x)	((x) & 0xff)

struct mmio_74xx_gpio_priv {
	struct bgpio_chip	bgc;
	unsigned int		flags;
};

static const struct of_device_id mmio_74xx_gpio_ids[] = {
	{
		.compatible	= "ti,741g125",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 1),
	},
	{
		.compatible	= "ti,742g125",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 2),
	},
	{
		.compatible	= "ti,74125",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 4),
	},
	{
		.compatible	= "ti,74365",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 6),
	},
	{
		.compatible	= "ti,74244",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 8),
	},
	{
		.compatible	= "ti,741624",
		.data		= (const void *)(MMIO_74XX_DIR_IN | 16),
	},
	{
		.compatible	= "ti,741g74",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 1),
	},
	{
		.compatible	= "ti,7474",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 2),
	},
	{
		.compatible	= "ti,74175",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 4),
	},
	{
		.compatible	= "ti,74174",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 6),
	},
	{
		.compatible	= "ti,74273",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 8),
	},
	{
		.compatible	= "ti,7416374",
		.data		= (const void *)(MMIO_74XX_DIR_OUT | 16),
	},
	{ }
};
MODULE_DEVICE_TABLE(of, mmio_74xx_gpio_ids);

static inline
struct mmio_74xx_gpio_priv *to_mmio_74xx_gpio_priv(struct gpio_chip *gc)
{
	struct bgpio_chip *bgc =
		container_of(gc, struct bgpio_chip, gc);

	return container_of(bgc, struct mmio_74xx_gpio_priv, bgc);
}

static int mmio_74xx_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	struct mmio_74xx_gpio_priv *priv = to_mmio_74xx_gpio_priv(gc);

	if (priv->flags & MMIO_74XX_DIR_OUT)
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static int mmio_74xx_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	struct mmio_74xx_gpio_priv *priv = to_mmio_74xx_gpio_priv(gc);

	return (priv->flags & MMIO_74XX_DIR_OUT) ? -ENOTSUPP : 0;
}

static int mmio_74xx_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct mmio_74xx_gpio_priv *priv = to_mmio_74xx_gpio_priv(gc);

	if (priv->flags & MMIO_74XX_DIR_OUT) {
		gc->ops->set(gc, gpio, val);
		return 0;
	}

	return -ENOTSUPP;
}

static int mmio_74xx_gpio_probe(struct device *dev)
{
	struct mmio_74xx_gpio_priv *priv;
	void __iomem *dat;
	int err;
	struct gpio_chip *gc;

	priv = xzalloc(sizeof(*priv));

	priv->flags = (uintptr_t)of_device_get_match_data(dev);

	dat = dev_request_mem_region(dev, 0);
	if (IS_ERR(dat))
		return PTR_ERR(dat);

	err = bgpio_init(&priv->bgc, dev,
			 DIV_ROUND_UP(MMIO_74XX_BIT_CNT(priv->flags), 8),
			 dat, NULL, NULL, NULL, NULL, 0);
	if (err)
		return err;

	gc = &priv->bgc.gc;
	gc->ops->direction_input = mmio_74xx_dir_in;
	gc->ops->direction_output = mmio_74xx_dir_out;
	gc->ops->get_direction = mmio_74xx_get_direction;
	gc->ngpio = MMIO_74XX_BIT_CNT(priv->flags);

	dev->priv = priv;

	return gpiochip_add(gc);
}

static struct driver mmio_74xx_gpio_driver = {
	.name = "74xx-mmio-gpio",
	.of_compatible	= DRV_OF_COMPAT(mmio_74xx_gpio_ids),
	.probe = mmio_74xx_gpio_probe,
};

coredevice_platform_driver(mmio_74xx_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexander Shiyan <shc_work@mail.ru>");
MODULE_DESCRIPTION("74xx MMIO GPIO driver");
