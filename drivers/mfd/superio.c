// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "superio: " fmt

#include <common.h>
#include <superio.h>
#include <regmap.h>

struct device_d *superio_func_add(struct superio_chip *siochip, const char *name)
{
	struct device_d *dev;
	int ret;

	dev = device_alloc(name, DEVICE_ID_DYNAMIC);
	dev->parent = siochip->dev;


	ret = platform_device_register(dev);
	if (ret)
		return NULL;

	return dev;
}
EXPORT_SYMBOL(superio_func_add);

static int superio_reg_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct superio_chip *siochip = ctx;

	siochip->enter(siochip->sioaddr);

	*val = superio_inb(siochip->sioaddr, reg);

	siochip->exit(siochip->sioaddr);

	return 0;
}

static int superio_reg_write(void *ctx, unsigned int reg, unsigned int val)
{
	struct superio_chip *siochip = ctx;

	siochip->enter(siochip->sioaddr);

	superio_outb(siochip->sioaddr, reg, val);

	siochip->exit(siochip->sioaddr);

	return 0;
}

static struct regmap_bus superio_regmap_bus = {
	.reg_write = superio_reg_write,
	.reg_read = superio_reg_read,
};

static struct regmap_config superio_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
	.max_register = 0xff,
};

void superio_chip_add(struct superio_chip *siochip)
{
	struct regmap *regmap;
	char *chipname;
	char str[5];
	int ret;

	chipname = xasprintf("superio-%04x:%04x@%02x",
			     siochip->vid, siochip->devid, siochip->sioaddr);
	siochip->dev = add_generic_device(chipname, DEVICE_ID_SINGLE, NULL,
					  siochip->sioaddr, 2, IORESOURCE_IO,
					  NULL);

	siochip->dev->priv = siochip;

	sprintf(str, "%04x", siochip->vid);
	dev_add_param_fixed(siochip->dev, "vendor", str);
	sprintf(str, "%04x", siochip->devid);
	dev_add_param_fixed(siochip->dev, "device", str);

	regmap = regmap_init(siochip->dev, &superio_regmap_bus, siochip,
			     &superio_regmap_config);
	if (IS_ERR(regmap))
		pr_warn("creating %s regmap failed: %pe\n", chipname, regmap);

	ret = regmap_register_cdev(regmap, chipname);
	if (ret)
		pr_warn("registering %s regmap cdev failed: %s\n",
			chipname, strerror(-ret));
}
EXPORT_SYMBOL(superio_chip_add);
