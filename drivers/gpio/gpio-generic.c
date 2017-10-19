/*
 * Generic driver for memory-mapped GPIO controllers.
 *
 * Based on linux driver by:
 *  Copyright 2008 MontaVista Software, Inc.
 *  Copyright 2008,2010 Anton Vorontsov <cbouatmailru@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <init.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/log2.h>
#include <linux/err.h>
#include <linux/basic_mmio_gpio.h>

static void bgpio_write8(void __iomem *reg, unsigned int data)
{
	writeb(data, reg);
}

static unsigned int bgpio_read8(void __iomem *reg)
{
	return readb(reg);
}

static void bgpio_write16(void __iomem *reg, unsigned int data)
{
	writew(data, reg);
}

static unsigned int bgpio_read16(void __iomem *reg)
{
	return readw(reg);
}

static void bgpio_write32(void __iomem *reg, unsigned int data)
{
	writel(data, reg);
}

static unsigned int bgpio_read32(void __iomem *reg)
{
	return readl(reg);
}

static unsigned int bgpio_pin2mask(struct bgpio_chip *bgc, unsigned int pin)
{
	return 1 << pin;
}

static unsigned int bgpio_pin2mask_be(struct bgpio_chip *bgc, unsigned int pin)
{
	return 1 << (bgc->bits - 1 - pin);
}

static int bgpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	return bgc->read_reg(bgc->reg_dat) & bgc->pin2mask(bgc, gpio);
}

static void bgpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned int mask = bgc->pin2mask(bgc, gpio);

	if (val)
		bgc->data |= mask;
	else
		bgc->data &= ~mask;

	bgc->write_reg(bgc->reg_dat, bgc->data);
}

static void bgpio_set_with_clear(struct gpio_chip *gc, unsigned int gpio,
				 int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned int mask = bgc->pin2mask(bgc, gpio);

	if (val)
		bgc->write_reg(bgc->reg_set, mask);
	else
		bgc->write_reg(bgc->reg_clr, mask);
}

static void bgpio_set_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned int mask = bgc->pin2mask(bgc, gpio);

	if (val)
		bgc->data |= mask;
	else
		bgc->data &= ~mask;

	bgc->write_reg(bgc->reg_set, bgc->data);
}

static int bgpio_simple_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	return 0;
}

static int bgpio_simple_dir_out(struct gpio_chip *gc, unsigned int gpio,
				int val)
{
	gc->ops->set(gc, gpio, val);

	return 0;
}

static int bgpio_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	bgc->dir &= ~bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	return 0;
}

static int bgpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	gc->ops->set(gc, gpio, val);

	bgc->dir |= bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	return 0;
}

static int bgpio_dir_in_inv(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	bgc->dir |= bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	return 0;
}

static int bgpio_dir_out_inv(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	gc->ops->set(gc, gpio, val);

	bgc->dir &= ~bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	return 0;
}

static int bgpio_setup_accessors(struct device_d *dev, struct bgpio_chip *bgc,
				 bool be)
{
	switch (bgc->bits) {
	case 8:
		bgc->read_reg	= bgpio_read8;
		bgc->write_reg	= bgpio_write8;
		break;
	case 16:
		bgc->read_reg	= bgpio_read16;
		bgc->write_reg	= bgpio_write16;
		break;
	case 32:
		bgc->read_reg	= bgpio_read32;
		bgc->write_reg	= bgpio_write32;
		break;
	default:
		dev_err(dev, "Unsupported data width %u bits\n", bgc->bits);
		return -EINVAL;
	}

	bgc->pin2mask = be ? bgpio_pin2mask_be : bgpio_pin2mask;

	return 0;
}

/*
 * Create the device and allocate the resources.  For setting GPIO's there are
 * three supported configurations:
 *
 *	- single input/output register resource (named "dat").
 *	- set/clear pair (named "set" and "clr").
 *	- single output register resource and single input resource ("set" and
 *	  dat").
 *
 * For the single output register, this drives a 1 by setting a bit and a zero
 * by clearing a bit.  For the set clr pair, this drives a 1 by setting a bit
 * in the set register and clears it by setting a bit in the clear register.
 * The configuration is detected by which resources are present.
 *
 * For setting the GPIO direction, there are three supported configurations:
 *
 *	- simple bidirection GPIO that requires no configuration.
 *	- an output direction register (named "dirout") where a 1 bit
 *	indicates the GPIO is an output.
 *	- an input direction register (named "dirin") where a 1 bit indicates
 *	the GPIO is an input.
 */
static int bgpio_setup_io(struct bgpio_chip *bgc,
			  void __iomem *dat,
			  void __iomem *set,
			  void __iomem *clr)
{
	if (!dat)
		return -EINVAL;

	bgc->reg_dat = dat;

	if (set && clr) {
		bgc->reg_set = set;
		bgc->reg_clr = clr;
		bgc->gc.ops->set = bgpio_set_with_clear;
	} else if (set && !clr) {
		bgc->reg_set = set;
		bgc->gc.ops->set = bgpio_set_set;
	} else
		bgc->gc.ops->set = bgpio_set;

	bgc->gc.ops->get = bgpio_get;

	return 0;
}

static int bgpio_setup_direction(struct bgpio_chip *bgc,
				 void __iomem *dirout,
				 void __iomem *dirin)
{
	if (dirout && dirin)
		return -EINVAL;

	if (dirout) {
		bgc->reg_dir = dirout;
		bgc->gc.ops->direction_output = bgpio_dir_out;
		bgc->gc.ops->direction_input = bgpio_dir_in;
	} else if (dirin) {
		bgc->reg_dir = dirin;
		bgc->gc.ops->direction_output = bgpio_dir_out_inv;
		bgc->gc.ops->direction_input = bgpio_dir_in_inv;
	} else {
		bgc->gc.ops->direction_output = bgpio_simple_dir_out;
		bgc->gc.ops->direction_input = bgpio_simple_dir_in;
	}

	return 0;
}

int bgpio_init(struct bgpio_chip *bgc, struct device_d *dev,
	       unsigned int sz, void __iomem *dat, void __iomem *set,
	       void __iomem *clr, void __iomem *dirout, void __iomem *dirin,
	       unsigned long flags)
{
	int ret;

	if ((sz > 4) || !is_power_of_2(sz))
		return -EINVAL;

	bgc->bits = sz * 8;
	bgc->gc.ngpio = bgc->bits;
	bgc->gc.base = -1;
	bgc->gc.dev = dev;
	bgc->gc.ops = &bgc->ops;

	ret = bgpio_setup_io(bgc, dat, set, clr);
	if (ret)
		return ret;

	ret = bgpio_setup_accessors(dev, bgc, flags & BGPIOF_BIG_ENDIAN);
	if (ret)
		return ret;

	ret = bgpio_setup_direction(bgc, dirout, dirin);
	if (ret)
		return ret;

	bgc->data = bgc->read_reg(bgc->reg_dat);

	if (bgc->gc.ops->set == bgpio_set_set && !(flags &
	    BGPIOF_UNREADABLE_REG_SET))
		bgc->data = bgc->read_reg(bgc->reg_set);

	if (bgc->reg_dir && !(flags & BGPIOF_UNREADABLE_REG_DIR))
		bgc->dir = bgc->read_reg(bgc->reg_dir);

	return ret;
}

void bgpio_remove(struct bgpio_chip *bgc)
{
	gpiochip_remove(&bgc->gc);
	free(bgc);
}

#ifdef CONFIG_GPIO_GENERIC_PLATFORM

static void __iomem *bgpio_map(struct device_d *dev, const char *name,
			       resource_size_t sane_sz, int *err)
{
	struct resource *r;
	struct resource *ret;

	*err = 0;

	r = dev_get_resource_by_name(dev, IORESOURCE_MEM, name);
	if (IS_ERR(r))
		return NULL;

	if (resource_size(r) != sane_sz) {
		*err = -EINVAL;
		return NULL;
	}

	ret = request_iomem_region(dev_name(dev), r->start, r->end);
	if (IS_ERR(ret)) {
		*err = PTR_ERR(ret);
		return NULL;
	}

	return IOMEM(ret->start);
}

static int bgpio_dev_probe(struct device_d *dev)
{
	struct resource *r;
	void __iomem *dat;
	void __iomem *set;
	void __iomem *clr;
	void __iomem *dirout;
	void __iomem *dirin;
	unsigned int sz;
	unsigned long flags = 0;
	int err;
	struct bgpio_chip *bgc;
	struct bgpio_pdata *pdata = dev->platform_data;

	r = dev_get_resource_by_name(dev, IORESOURCE_MEM, "dat");
	if (IS_ERR(r))
		return PTR_ERR(r);

	sz = resource_size(r);

	dat = bgpio_map(dev, "dat", sz, &err);
	if (!dat)
		return err ? err : -EINVAL;

	set = bgpio_map(dev, "set", sz, &err);
	if (err)
		return err;

	clr = bgpio_map(dev, "clr", sz, &err);
	if (err)
		return err;

	dirout = bgpio_map(dev, "dirout", sz, &err);
	if (err)
		return err;

	dirin = bgpio_map(dev, "dirin", sz, &err);
	if (err)
		return err;

	dev_get_drvdata(dev, (const void **)&flags);

	bgc = xzalloc(sizeof(struct bgpio_chip));

	err = bgpio_init(bgc, dev, sz, dat, set, clr, dirout, dirin, flags);
	if (err)
		return err;

	if (pdata) {
		bgc->gc.base = pdata->base;
		if (pdata->ngpio > 0)
			bgc->gc.ngpio = pdata->ngpio;
	}

	dev->priv = bgc;

	return gpiochip_add(&bgc->gc);
}

static void bgpio_dev_remove(struct device_d *dev)
{
	struct bgpio_chip *bgc = dev->priv;

	bgpio_remove(bgc);
}

static struct platform_device_id bgpio_id_table[] = {
	{
		.name		= "basic-mmio-gpio",
		.driver_data	= 0,
	},
	{
		.name		= "basic-mmio-gpio-be",
		.driver_data	= BGPIOF_BIG_ENDIAN,
	},
	{ }
};

static struct of_device_id __maybe_unused bgpio_of_match[] = {
	{
		.compatible = "wd,mbl-gpio",
	}, {
		/* sentinel */
	}
};

static struct driver_d bgpio_driver = {
	.name		= "basic-mmio-gpio",
	.id_table	= bgpio_id_table,
	.of_compatible	= DRV_OF_COMPAT(bgpio_of_match),
	.probe		= bgpio_dev_probe,
	.remove		= bgpio_dev_remove,
};

static int bgpio_register(void)
{
	return platform_driver_register(&bgpio_driver);
}
coredevice_initcall(bgpio_register);

#endif

MODULE_DESCRIPTION("Driver for basic memory-mapped GPIO controllers");
MODULE_AUTHOR("Anton Vorontsov <cbouatmailru@gmail.com>");
MODULE_LICENSE("GPL");
