// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 MontaVista Software, Inc.
// SPDX-FileCopyrightText: 2008,2010 Anton Vorontsov <cbouatmailru@gmail.com>

/*
 * Generic driver for memory-mapped GPIO controllers, based on the Linux driver.
 */

#include <init.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <errno.h>
#include <linux/log2.h>
#include <linux/ioport.h>
#include <io.h>
#include <linux/basic_mmio_gpio.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <driver.h>
#include <of.h>
#include <of_device.h>

static void bgpio_write8(void __iomem *reg, unsigned long data)
{
	writeb(data, reg);
}

static unsigned long bgpio_read8(void __iomem *reg)
{
	return readb(reg);
}

static void bgpio_write16(void __iomem *reg, unsigned long data)
{
	writew(data, reg);
}

static unsigned long bgpio_read16(void __iomem *reg)
{
	return readw(reg);
}

static void bgpio_write32(void __iomem *reg, unsigned long data)
{
	writel(data, reg);
}

static unsigned long bgpio_read32(void __iomem *reg)
{
	return readl(reg);
}

#if BITS_PER_LONG >= 64
static void bgpio_write64(void __iomem *reg, unsigned long data)
{
	writeq(data, reg);
}

static unsigned long bgpio_read64(void __iomem *reg)
{
	return readq(reg);
}
#endif /* BITS_PER_LONG >= 64 */

static void bgpio_write16be(void __iomem *reg, unsigned long data)
{
	iowrite16be(data, reg);
}

static unsigned long bgpio_read16be(void __iomem *reg)
{
	return ioread16be(reg);
}

static void bgpio_write32be(void __iomem *reg, unsigned long data)
{
	iowrite32be(data, reg);
}

static unsigned long bgpio_read32be(void __iomem *reg)
{
	return ioread32be(reg);
}

static unsigned long bgpio_line2mask(struct bgpio_chip *bgc, unsigned int line)
{
	if (bgc->be_bits)
		return BIT(bgc->bits - 1 - line);
	return BIT(line);
}

static int bgpio_get_set(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long pinmask = bgpio_line2mask(bgc, gpio);
	bool dir = !!(bgc->dir & pinmask);

	if (dir)
		return !!(bgc->read_reg(bgc->reg_set) & pinmask);
	else
		return !!(bgc->read_reg(bgc->reg_dat) & pinmask);
}

static int bgpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	return !!(bgc->read_reg(bgc->reg_dat) & bgpio_line2mask(bgc, gpio));
}

static void bgpio_set_none(struct gpio_chip *gc, unsigned int gpio, int val)
{
}

static void bgpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long mask = bgpio_line2mask(bgc, gpio);

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
	unsigned long mask = bgpio_line2mask(bgc, gpio);

	if (val)
		bgc->write_reg(bgc->reg_set, mask);
	else
		bgc->write_reg(bgc->reg_clr, mask);
}

static void bgpio_set_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long mask = bgpio_line2mask(bgc, gpio);

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

static int bgpio_dir_out_err(struct gpio_chip *gc, unsigned int gpio,
				int val)
{
	return -EINVAL;
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

	bgc->dir &= ~bgpio_line2mask(bgc, gpio);

	if (bgc->reg_dir_in)
		bgc->write_reg(bgc->reg_dir_in, ~bgc->dir);
	if (bgc->reg_dir_out)
		bgc->write_reg(bgc->reg_dir_out, bgc->dir);

	return 0;
}

static int bgpio_get_dir(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	/* Return 0 if output, 1 if input */
	if (bgc->dir_unreadable) {
		if (bgc->dir & bgpio_line2mask(bgc, gpio))
			return GPIOF_DIR_OUT;
		return GPIOF_DIR_IN;
	}

	if (bgc->reg_dir_out) {
		if (bgc->read_reg(bgc->reg_dir_out) & bgpio_line2mask(bgc, gpio))
			return GPIOF_DIR_OUT;
		return GPIOF_DIR_IN;
	}

	if (bgc->reg_dir_in)
		if (!(bgc->read_reg(bgc->reg_dir_in) & bgpio_line2mask(bgc, gpio)))
			return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static void bgpio_dir_out(struct bgpio_chip *bgc, unsigned int gpio, int val)
{
	bgc->dir |= bgpio_line2mask(bgc, gpio);

	if (bgc->reg_dir_in)
		bgc->write_reg(bgc->reg_dir_in, ~bgc->dir);
	if (bgc->reg_dir_out)
		bgc->write_reg(bgc->reg_dir_out, bgc->dir);
}

static int bgpio_dir_out_dir_first(struct gpio_chip *gc, unsigned int gpio,
				   int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	bgpio_dir_out(bgc, gpio, val);
	gc->ops->set(gc, gpio, val);
	return 0;
}

static int bgpio_dir_out_val_first(struct gpio_chip *gc, unsigned int gpio,
				   int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	gc->ops->set(gc, gpio, val);
	bgpio_dir_out(bgc, gpio, val);
	return 0;
}

static int bgpio_setup_accessors(struct device *dev,
				 struct bgpio_chip *bgc,
				 bool byte_be)
{

	switch (bgc->bits) {
	case 8:
		bgc->read_reg	= bgpio_read8;
		bgc->write_reg	= bgpio_write8;
		break;
	case 16:
		if (byte_be) {
			bgc->read_reg	= bgpio_read16be;
			bgc->write_reg	= bgpio_write16be;
		} else {
			bgc->read_reg	= bgpio_read16;
			bgc->write_reg	= bgpio_write16;
		}
		break;
	case 32:
		if (byte_be) {
			bgc->read_reg	= bgpio_read32be;
			bgc->write_reg	= bgpio_write32be;
		} else {
			bgc->read_reg	= bgpio_read32;
			bgc->write_reg	= bgpio_write32;
		}
		break;
#if BITS_PER_LONG >= 64
	case 64:
		if (byte_be) {
			dev_err(dev,
				"64 bit big endian byte order unsupported\n");
			return -EINVAL;
		} else {
			bgc->read_reg	= bgpio_read64;
			bgc->write_reg	= bgpio_write64;
		}
		break;
#endif /* BITS_PER_LONG >= 64 */
	default:
		dev_err(dev, "unsupported data width %u bits\n", bgc->bits);
		return -EINVAL;
	}

	return 0;
}

/*
 * Create the device and allocate the resources.  For setting GPIO's there are
 * three supported configurations:
 *
 *	- single input/output register resource (named "dat").
 *	- set/clear pair (named "set" and "clr").
 *	- single output register resource and single input resource ("set" and
 *	dat").
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
			  void __iomem *clr,
			  unsigned long flags)
{
	struct gpio_ops *ops = bgc->gc.ops;

	bgc->reg_dat = dat;
	if (!bgc->reg_dat)
		return -EINVAL;

	if (set && clr) {
		bgc->reg_set = set;
		bgc->reg_clr = clr;
		ops->set = bgpio_set_with_clear;
	} else if (set && !clr) {
		bgc->reg_set = set;
		ops->set = bgpio_set_set;
	} else if (flags & BGPIOF_NO_OUTPUT) {
		ops->set = bgpio_set_none;
	} else {
		ops->set = bgpio_set;
	}

	if (!(flags & BGPIOF_UNREADABLE_REG_SET) && (flags & BGPIOF_READ_OUTPUT_REG_SET))
		ops->get = bgpio_get_set;
	else
		ops->get = bgpio_get;

	return 0;
}

static int bgpio_setup_direction(struct bgpio_chip *bgc,
				 void __iomem *dirout,
				 void __iomem *dirin,
				 unsigned long flags)
{
	struct gpio_ops *ops = bgc->gc.ops;

	if (dirout || dirin) {
		bgc->reg_dir_out = dirout;
		bgc->reg_dir_in = dirin;
		if (flags & BGPIOF_NO_SET_ON_INPUT)
			ops->direction_output = bgpio_dir_out_dir_first;
		else
			ops->direction_output = bgpio_dir_out_val_first;
		ops->direction_input = bgpio_dir_in;
		ops->get_direction = bgpio_get_dir;
	} else {
		if (flags & BGPIOF_NO_OUTPUT)
			ops->direction_output = bgpio_dir_out_err;
		else
			ops->direction_output = bgpio_simple_dir_out;
		ops->direction_input = bgpio_simple_dir_in;
	}

	return 0;
}

static int bgpio_request(struct gpio_chip *chip, unsigned gpio_pin)
{
	if (gpio_pin < chip->ngpio)
		return 0;

	return -EINVAL;
}

/**
 * bgpio_init() - Initialize generic GPIO accessor functions
 * @bgc: the GPIO chip to set up
 * @dev: the parent device of the new GPIO chip (compulsory)
 * @sz: the size (width) of the MMIO registers in bytes, typically 1, 2 or 4
 * @dat: MMIO address for the register to READ the value of the GPIO lines, it
 *	is expected that a 1 in the corresponding bit in this register means the
 *	line is asserted
 * @set: MMIO address for the register to SET the value of the GPIO lines, it is
 *	expected that we write the line with 1 in this register to drive the GPIO line
 *	high.
 * @clr: MMIO address for the register to CLEAR the value of the GPIO lines, it is
 *	expected that we write the line with 1 in this register to drive the GPIO line
 *	low. It is allowed to leave this address as NULL, in that case the SET register
 *	will be assumed to also clear the GPIO lines, by actively writing the line
 *	with 0.
 * @dirout: MMIO address for the register to set the line as OUTPUT. It is assumed
 *	that setting a line to 1 in this register will turn that line into an
 *	output line. Conversely, setting the line to 0 will turn that line into
 *	an input.
 * @dirin: MMIO address for the register to set this line as INPUT. It is assumed
 *	that setting a line to 1 in this register will turn that line into an
 *	input line. Conversely, setting the line to 0 will turn that line into
 *	an output.
 * @flags: Different flags that will affect the behaviour of the device, such as
 *	endianness etc.
 */
int bgpio_init(struct bgpio_chip *bgc, struct device *dev,
	       unsigned int sz, void __iomem *dat, void __iomem *set,
	       void __iomem *clr, void __iomem *dirout, void __iomem *dirin,
	       unsigned long flags)
{
	struct gpio_ops *ops = &bgc->ops;
	int ret;

	if (!is_power_of_2(sz))
		return -EINVAL;

	bgc->bits = sz * 8;
	if (bgc->bits > BITS_PER_LONG)
		return -EINVAL;

	bgc->gc.base = -1;
	bgc->gc.ngpio = bgc->bits;
	bgc->gc.dev = dev;
	bgc->gc.ops = ops;
	ops->request = bgpio_request;
	bgc->be_bits = !!(flags & BGPIOF_BIG_ENDIAN);

	ret = bgpio_setup_io(bgc, dat, set, clr, flags);
	if (ret)
		return ret;

	ret = bgpio_setup_accessors(dev, bgc, flags & BGPIOF_BIG_ENDIAN_BYTE_ORDER);
	if (ret)
		return ret;

	ret = bgpio_setup_direction(bgc, dirout, dirin, flags);
	if (ret)
		return ret;

	bgc->data = bgc->read_reg(bgc->reg_dat);
	if (ops->set == bgpio_set_set &&
			!(flags & BGPIOF_UNREADABLE_REG_SET))
		bgc->data = bgc->read_reg(bgc->reg_set);

	if (flags & BGPIOF_UNREADABLE_REG_DIR)
		bgc->dir_unreadable = true;

	/*
	 * Inspect hardware to find initial direction setting.
	 */
	if ((bgc->reg_dir_out || bgc->reg_dir_in) &&
	    !(flags & BGPIOF_UNREADABLE_REG_DIR)) {
		if (bgc->reg_dir_out)
			bgc->dir = bgc->read_reg(bgc->reg_dir_out);
		else if (bgc->reg_dir_in)
			bgc->dir = ~bgc->read_reg(bgc->reg_dir_in);
		/*
		 * If we have two direction registers, synchronise
		 * input setting to output setting, the library
		 * can not handle a line being input and output at
		 * the same time.
		 */
		if (bgc->reg_dir_out && bgc->reg_dir_in)
			bgc->write_reg(bgc->reg_dir_in, ~bgc->dir);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(bgpio_init);

void bgpio_remove(struct bgpio_chip *bgc)
{
	gpiochip_remove(&bgc->gc);
	free(bgc);
}
EXPORT_SYMBOL_GPL(bgpio_remove);

#ifdef CONFIG_GPIO_GENERIC_PLATFORM

static void __iomem *bgpio_map(struct device *dev,
			       const char *name,
			       resource_size_t sane_sz)
{
	struct resource *r;
	resource_size_t sz;

	r = dev_request_mem_resource_by_name(dev, name);
	if (IS_ERR(r))
		return NULL;

	sz = resource_size(r);
	if (sz != sane_sz)
		return IOMEM_ERR_PTR(-EINVAL);

	return IOMEM(r->start);
}

static const struct of_device_id bgpio_of_match[];

static struct bgpio_pdata *bgpio_parse_dt(struct device *dev,
					  unsigned long *flags)
{
	struct bgpio_pdata *pdata;

	if (!of_match_device(bgpio_of_match, dev))
		return NULL;

	pdata = xzalloc(sizeof(struct bgpio_pdata));

	pdata->base = -1;

	if (of_device_is_big_endian(dev->of_node))
		*flags |= BGPIOF_BIG_ENDIAN_BYTE_ORDER;

	if (of_property_read_bool(dev->of_node, "no-output"))
		*flags |= BGPIOF_NO_OUTPUT;

	return pdata;
}

static int bgpio_dev_probe(struct device *dev)
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
	struct bgpio_pdata *pdata;

	pdata = bgpio_parse_dt(dev, &flags);
	if (IS_ERR(pdata))
		return PTR_ERR(pdata);

	r = dev_get_resource_by_name(dev, IORESOURCE_MEM, "dat");
	if (!r)
		return -EINVAL;

	sz = resource_size(r);

	dat = bgpio_map(dev, "dat", sz);
	if (IS_ERR(dat))
		return PTR_ERR(dat);

	set = bgpio_map(dev, "set", sz);
	if (IS_ERR(set))
		return PTR_ERR(set);

	clr = bgpio_map(dev, "clr", sz);
	if (IS_ERR(clr))
		return PTR_ERR(clr);

	dirout = bgpio_map(dev, "dirout", sz);
	if (IS_ERR(dirout))
		return PTR_ERR(dirout);

	dirin = bgpio_map(dev, "dirin", sz);
	if (IS_ERR(dirin))
		return PTR_ERR(dirin);

	bgc = xzalloc(sizeof(struct bgpio_chip));

	err = bgpio_init(bgc, dev, sz, dat, set, clr, dirout, dirin, flags);
	if (err)
		return err;

	bgc->gc.base = pdata->base;
	bgc->gc.dev = dev;
	bgc->gc.ops = &bgc->ops;
	if (pdata->ngpio > 0)
		bgc->gc.ngpio = pdata->ngpio;

	dev->priv = bgc;

	return gpiochip_add(&bgc->gc);
}

static void bgpio_dev_remove(struct device *dev)
{
	struct bgpio_chip *bgc = dev->priv;

	bgpio_remove(bgc);
}

static const struct of_device_id bgpio_of_match[] = {
	{
		.compatible = "wd,mbl-gpio",
	},
	{
		.compatible = "brcm,bcm6345-gpio"
	},
	{
		.compatible = "ni,169445-nand-gpio"
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, bgpio_of_match);

static struct driver bgpio_driver = {
	.name		= "basic-mmio-gpio",
	.of_compatible	= bgpio_of_match,
	.probe		= bgpio_dev_probe,
	.remove		= bgpio_dev_remove,
};

coredevice_platform_driver(bgpio_driver);

#endif

MODULE_DESCRIPTION("Driver for basic memory-mapped GPIO controllers");
MODULE_AUTHOR("Anton Vorontsov <cbouatmailru@gmail.com>");
MODULE_LICENSE("GPL");
