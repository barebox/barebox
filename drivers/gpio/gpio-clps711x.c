// SPDX-License-Identifier: GPL-2.0-or-later
/* Author: Alexander Shiyan <shc_work@mail.ru> */

#include <init.h>
#include <common.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/basic_mmio_gpio.h>

static int clps711x_gpio_probe(struct device *dev)
{
	struct resource *iores;
	int err, id = of_alias_get_id(dev->of_node, "gpio");
	void __iomem *dat, *dir = NULL, *dir_inv = NULL;
	struct bgpio_chip *bgc;

	if (id < 0 || id > 4)
		return -ENODEV;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	dat = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	switch (id) {
	case 3:
		dir_inv = IOMEM(iores->start);
		break;
	default:
		dir = IOMEM(iores->start);
		break;
	}

	bgc = xzalloc(sizeof(struct bgpio_chip));

	err = bgpio_init(bgc, dev, 1, dat, NULL, NULL, dir, dir_inv, 0);
	if (err)
		goto out_err;

	bgc->gc.base = id * 8;
	switch (id) {
	case 4:
		bgc->gc.ngpio = 3;
		break;
	default:
		break;
	}

	err = gpiochip_add(&bgc->gc);

out_err:
	if (err)
		free(bgc);

	return err;
}

static const struct of_device_id __maybe_unused clps711x_gpio_dt_ids[] = {
	{ .compatible = "cirrus,ep7209-gpio", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, clps711x_gpio_dt_ids);

static struct driver clps711x_gpio_driver = {
	.name		= "clps711x-gpio",
	.probe		= clps711x_gpio_probe,
	.of_compatible	= DRV_OF_COMPAT(clps711x_gpio_dt_ids),
};
coredevice_platform_driver(clps711x_gpio_driver);
