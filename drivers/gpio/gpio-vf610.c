// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Toradex AG (Stefan Agner <stefan@agner.ch>)

/*
 * vf610 GPIO support through PORT and GPIO module
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <gpio.h>
#include <init.h>
#include <pinctrl.h>

#define VF610_GPIO_PER_PORT	32
#define PINCTRL_BASE		2
#define COUNT 3

struct vf610_gpio_port {
	struct gpio_chip chip;
	void __iomem *gpio_base;
	unsigned int pinctrl_base;
};

#define GPIO_PDOR		0x00
#define GPIO_PSOR		0x04
#define GPIO_PCOR		0x08
#define GPIO_PTOR		0x0c
#define GPIO_PDIR		0x10

static const struct of_device_id vf610_gpio_dt_ids[] = {
	{ .compatible = "fsl,vf610-gpio" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vf610_gpio_dt_ids);


static int vf610_gpio_get_value(struct gpio_chip *chip, unsigned int gpio)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);

	return !!(readl(port->gpio_base + GPIO_PDIR) & BIT(gpio));
}

static void vf610_gpio_set_value(struct gpio_chip *chip,
				  unsigned int gpio, int val)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);
	unsigned long mask = BIT(gpio);

	writel(mask, port->gpio_base + ((val) ? GPIO_PSOR : GPIO_PCOR));
}

static int vf610_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);

	return pinctrl_gpio_direction_input(port->pinctrl_base + gpio);
}

static int vf610_gpio_direction_output(struct gpio_chip *chip, unsigned gpio,
				       int value)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);

	vf610_gpio_set_value(chip, gpio, value);

	return pinctrl_gpio_direction_output(port->pinctrl_base + gpio);
}

static int vf610_gpio_get_direction(struct gpio_chip *chip, unsigned gpio)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);

	return pinctrl_gpio_get_direction(port->pinctrl_base + gpio);
}

static struct gpio_ops vf610_gpio_ops = {
	.direction_input = vf610_gpio_direction_input,
	.direction_output = vf610_gpio_direction_output,
	.get = vf610_gpio_get_value,
	.set = vf610_gpio_set_value,
	.get_direction = vf610_gpio_get_direction,
};

static int vf610_gpio_probe(struct device *dev)
{
	int ret, size;
	struct resource *iores;
	struct vf610_gpio_port *port;
	const __be32 *gpio_ranges;

	port = xzalloc(sizeof(*port));

	gpio_ranges = of_get_property(dev->of_node, "gpio-ranges", &size);
	if (!gpio_ranges) {
		dev_err(dev, "Couldn't read 'gpio-ranges' propery of %s\n",
			dev->of_node->full_name);
		ret = -EINVAL;
		goto free_port;
	}

	port->pinctrl_base = be32_to_cpu(gpio_ranges[PINCTRL_BASE]);
	port->chip.ngpio   = be32_to_cpu(gpio_ranges[COUNT]);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		dev_dbg(dev, "Failed to request memory resource\n");
		goto free_port;
	}

	port->gpio_base = IOMEM(iores->start);

	port->chip.ops  = &vf610_gpio_ops;
	if (dev->id < 0) {
		port->chip.base = of_alias_get_id(dev->of_node, "gpio");
		if (port->chip.base < 0) {
			ret = port->chip.base;
			dev_dbg(dev, "Failed to get GPIO alias\n");
			goto free_port;
		}
	} else {
		port->chip.base = dev->id;
	}


	port->chip.base *= VF610_GPIO_PER_PORT;
	port->chip.dev = dev;

	return gpiochip_add(&port->chip);

free_port:
	free(port);
	return ret;
}

static struct driver vf610_gpio_driver = {
	.name	= "gpio-vf610",
	.probe  = vf610_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(vf610_gpio_dt_ids),
};

postcore_platform_driver(vf610_gpio_driver);
