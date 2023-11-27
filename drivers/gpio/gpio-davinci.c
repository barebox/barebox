// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2006-2007 David Brownell
// SPDX-FileCopyrightText: 2007 MontaVista Software, Inc. <source@mvista.com>
// SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com>

/*
 * TI DaVinci GPIO Support
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

struct davinci_gpio_regs {
	u32	dir;
	u32	out_data;
	u32	set_data;
	u32	clr_data;
	u32	in_data;
	u32	set_rising;
	u32	clr_rising;
	u32	set_falling;
	u32	clr_falling;
	u32	intstat;
};

struct davinci_gpio_controller {
	struct gpio_chip	chip;
	/* Serialize access to GPIO registers */
	void __iomem		*regs;
};

#define chip2controller(chip)	\
	container_of(chip, struct davinci_gpio_controller, chip)

static struct davinci_gpio_regs __iomem *gpio2regs(struct davinci_gpio_controller *d,
						   unsigned gpio)
{
	void __iomem *ptr;

	if (gpio < 32 * 1)
		ptr = d->regs + 0x10;
	else if (gpio < 32 * 2)
		ptr = d->regs + 0x38;
	else if (gpio < 32 * 3)
		ptr = d->regs + 0x60;
	else if (gpio < 32 * 4)
		ptr = d->regs + 0x88;
	else if (gpio < 32 * 5)
		ptr = d->regs + 0xb0;
	else
		ptr = NULL;
	return ptr;
}

static inline u32 __gpio_mask(unsigned gpio)
{
	return 1 << (gpio % 32);
}

static int davinci_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = gpio2regs(d, offset);

	return ((readl_relaxed(&g->dir)) & __gpio_mask(offset)) ?
		GPIOF_DIR_IN : GPIOF_DIR_OUT;
}

static inline int __davinci_direction(struct gpio_chip *chip,
			unsigned offset, bool out, int value)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = gpio2regs(d, offset);
	u32 temp;
	u32 mask = __gpio_mask(offset);

	temp = readl_relaxed(&g->dir);
	if (out) {
		temp &= ~mask;
		writel_relaxed(mask, value ? &g->set_data : &g->clr_data);
	} else {
		temp |= mask;
	}
	writel_relaxed(temp, &g->dir);

	return 0;
}

static int davinci_direction_in(struct gpio_chip *chip, unsigned offset)
{
	return __davinci_direction(chip, offset, false, 0);
}

static int
davinci_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	return __davinci_direction(chip, offset, true, value);
}

/*
 * Read the pin's value (works even if it's set up as output);
 * returns zero/nonzero.
 *
 * Note that changes are synched to the GPIO clock, so reading values back
 * right after you've set them may give old values.
 */
static int davinci_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = gpio2regs(d, offset);

	return (__gpio_mask(offset) & readl_relaxed(&g->in_data)) ? 1 : 0;
}

/*
 * Assuming the pin is muxed as a gpio output, set its output value.
 */
static void
davinci_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = gpio2regs(d, offset);

	writel_relaxed(__gpio_mask(offset), value ? &g->set_data : &g->clr_data);
}

static struct gpio_ops davinci_gpio_ops = {
	.direction_input = davinci_direction_in,
	.direction_output = davinci_direction_out,
	.get_direction = davinci_get_direction,
	.get = davinci_gpio_get,
	.set = davinci_gpio_set,
};

static int davinci_gpio_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *gpio_base;
	int ret;
	u32 val;
	unsigned ngpio;
	struct davinci_gpio_controller *chips;
	struct gpio_chip *gc;

	ret = of_property_read_u32(dev->of_node, "ti,ngpio", &val);
	if (ret) {
		dev_err(dev, "could not read 'ti,ngpio' property\n");
		return -EINVAL;
	}

	ngpio = val;

	if (WARN_ON(ARCH_NR_GPIOS < ngpio))
		ngpio = ARCH_NR_GPIOS;

	chips = xzalloc(sizeof(*chips));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	gpio_base = IOMEM(iores->start);

	gc = &chips->chip;
	gc->ops = &davinci_gpio_ops;
	gc->dev = dev;
	gc->ngpio = ngpio;
	gc->base = -1;

	chips->regs = gpio_base;

	gpiochip_add(gc);

	return 0;
}

static struct of_device_id davinci_gpio_ids[] = {
	{ .compatible = "ti,keystone-gpio", },
	{ .compatible = "ti,am654-gpio", },
	{ .compatible = "ti,dm6441-gpio", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, davinci_gpio_ids);

static struct driver davinci_gpio_driver = {
	.name		= "davinci_gpio",
	.probe		= davinci_gpio_probe,
	.of_compatible	= DRV_OF_COMPAT(davinci_gpio_ids),
};

coredevice_platform_driver(davinci_gpio_driver);
