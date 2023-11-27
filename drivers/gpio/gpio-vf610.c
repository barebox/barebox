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
	bool have_paddr;
	bool need_pinctrl;
};

#define GPIO_PDOR		0x00
#define GPIO_PSOR		0x04
#define GPIO_PCOR		0x08
#define GPIO_PTOR		0x0c
#define GPIO_PDIR		0x10
#define GPIO_PDDR		0x14

struct fsl_gpio_soc_data {
	/* SoCs has a Port Data Direction Register (PDDR) */
	bool have_paddr;
	bool need_pinctrl;
};

static const struct fsl_gpio_soc_data vf610_data = {
	.need_pinctrl = true,
};

static const struct fsl_gpio_soc_data imx_data = {
	.have_paddr = true,
};

static const struct of_device_id vf610_gpio_dt_ids[] = {
	{ .compatible = "fsl,vf610-gpio", .data = &vf610_data, },
	{ .compatible = "fsl,imx7ulp-gpio", .data = &imx_data, },
	{ .compatible = "fsl,imx8ulp-gpio", .data = &imx_data, },
	{ .compatible = "fsl,imx93-gpio", .data = &imx_data, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vf610_gpio_dt_ids);


static int vf610_gpio_get_value(struct gpio_chip *chip, unsigned int gpio)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);
	unsigned long mask = BIT(gpio);
	unsigned long offset = GPIO_PDIR;

	if (port->have_paddr) {
		mask &= readl(port->gpio_base + GPIO_PDDR);
		if (mask)
			offset = GPIO_PDOR;
	}

	return !!(readl(port->gpio_base + offset) & BIT(gpio));
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
	unsigned long mask = BIT(gpio);
	u32 val;

	if (port->have_paddr) {
		val = readl(port->gpio_base + GPIO_PDDR);
		val &= ~mask;
		writel(val, port->gpio_base + GPIO_PDDR);
	}

	if (port->need_pinctrl)
		return pinctrl_gpio_direction_input(port->pinctrl_base + gpio);

	return 0;
}

static int vf610_gpio_direction_output(struct gpio_chip *chip, unsigned gpio,
				       int value)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);
	unsigned long mask = BIT(gpio);
	u32 val;

	vf610_gpio_set_value(chip, gpio, value);

	if (port->have_paddr) {
		val = readl(port->gpio_base + GPIO_PDDR);
		val |= mask;
		writel(val, port->gpio_base + GPIO_PDDR);
	}

	if (port->need_pinctrl)
		return pinctrl_gpio_direction_output(port->pinctrl_base + gpio);

	return 0;
}

static int vf610_gpio_get_direction(struct gpio_chip *chip, unsigned gpio)
{
	struct vf610_gpio_port *port =
		container_of(chip, struct vf610_gpio_port, chip);
	u32 val;

	if (port->have_paddr) {
		val = readl(port->gpio_base + GPIO_PDDR);
		if (val & BIT(gpio))
			return GPIOF_DIR_OUT;
		else
			return GPIOF_DIR_IN;
	}

	if (port->need_pinctrl)
		return pinctrl_gpio_get_direction(port->pinctrl_base + gpio);

	return 0;
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
	struct fsl_gpio_soc_data *devtype;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	port = xzalloc(sizeof(*port));

	gpio_ranges = of_get_property(dev->of_node, "gpio-ranges", &size);
	if (!gpio_ranges) {
		dev_err(dev, "Couldn't read 'gpio-ranges' propery of %pOF\n",
			dev->of_node);
		ret = -EINVAL;
		goto free_port;
	}

	port->have_paddr = devtype->have_paddr;
	port->need_pinctrl = devtype->need_pinctrl;

	port->pinctrl_base = be32_to_cpu(gpio_ranges[PINCTRL_BASE]);
	port->chip.ngpio   = be32_to_cpu(gpio_ranges[COUNT]);

	/*
	 * Some old bindings have two register ranges. When we have two ranges
	 * the GPIO base is in the second range. With only one range the GPIO
	 * base is at offset 0x40.
	 */
	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores)) {
		iores = dev_request_mem_resource(dev, 0);
		if (IS_ERR(iores)) {
			ret = PTR_ERR(iores);
			dev_dbg(dev, "Failed to request memory resource\n");
			goto free_port;
		} else {
			port->gpio_base = IOMEM(iores->start) + 0x40;
		}
	} else {
		port->gpio_base = IOMEM(iores->start);
	}

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
