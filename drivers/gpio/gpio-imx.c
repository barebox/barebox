// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2004 Sascha Hauer, Synertronixx GmbH

/*
 * arch/arm/mach-imx/gpio.c
 *
 * Common code for i.MX machines
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <gpio.h>
#include <init.h>

struct imx_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
	struct imx_gpio_regs *regs;
};

struct imx_gpio_regs {
	unsigned int dr;
	unsigned int gdir;
	unsigned int psr;
};

static struct imx_gpio_regs regs_imx1 = {
	.dr = 0x1c,
	.gdir = 0x00,
	.psr = 0x24,
};

static struct imx_gpio_regs regs_imx31 = {
	.dr = 0x00,
	.gdir = 0x04,
	.psr = 0x08,
};

static void imx_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	if (!base)
		return;

	val = readl(base + imxgpio->regs->dr);

	if (value)
		val |= 1 << gpio;
	else
		val &= ~(1 << gpio);

	writel(val, base + imxgpio->regs->dr);
}

static int imx_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	if (!base)
		return -EINVAL;

	val = readl(base + imxgpio->regs->gdir);
	val &= ~(1 << gpio);
	writel(val, base + imxgpio->regs->gdir);

	return 0;
}


static int imx_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	imx_gpio_set_value(chip, gpio, value);

	val = readl(base + imxgpio->regs->gdir);
	val |= 1 << gpio;
	writel(val, base + imxgpio->regs->gdir);

	return 0;
}

static int imx_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val;

	val = readl(base + imxgpio->regs->psr);

	return val & (1 << gpio) ? 1 : 0;
}

static int imx_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct imx_gpio_chip *imxgpio = container_of(chip, struct imx_gpio_chip, chip);
	void __iomem *base = imxgpio->base;
	u32 val = readl(base + imxgpio->regs->gdir);

	return (val & (1 << offset)) ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static struct gpio_ops imx_gpio_ops = {
	.direction_input = imx_gpio_direction_input,
	.direction_output = imx_gpio_direction_output,
	.get = imx_gpio_get_value,
	.set = imx_gpio_set_value,
	.get_direction = imx_get_direction,
};

static int imx_gpio_probe(struct device *dev)
{
	struct resource *iores;
	struct imx_gpio_chip *imxgpio;
	struct imx_gpio_regs *regs;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&regs);
	if (ret)
		return ret;

	imxgpio = xzalloc(sizeof(*imxgpio));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	imxgpio->base = IOMEM(iores->start);
	imxgpio->chip.ops = &imx_gpio_ops;
	if (dev->id < 0) {
		imxgpio->chip.base = of_alias_get_id(dev->of_node, "gpio");
		if (imxgpio->chip.base < 0)
			return imxgpio->chip.base;
		imxgpio->chip.base *= 32;
	} else {
		imxgpio->chip.base = dev->id * 32;
	}
	imxgpio->chip.ngpio = 32;
	imxgpio->chip.dev = dev;
	imxgpio->regs = regs;
	gpiochip_add(&imxgpio->chip);

	dev_dbg(dev, "probed gpiochip%d with base %d\n", dev->id, imxgpio->chip.base);

	return 0;
}

static __maybe_unused struct of_device_id imx_gpio_dt_ids[] = {
	{
		.compatible = "fsl,imx1-gpio",
		.data = &regs_imx1,
	}, {
		.compatible = "fsl,imx21-gpio",
		.data = &regs_imx1,
	}, {
		.compatible = "fsl,imx27-gpio",
		.data = &regs_imx1,
	}, {
		.compatible = "fsl,imx31-gpio",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx35-gpio",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx51-gpio",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx53-gpio",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx6q-gpio",
		.data = &regs_imx31,
	},  {
		.compatible = "fsl,imx8mq-gpio",
		.data = &regs_imx31,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, imx_gpio_dt_ids);

static struct platform_device_id imx_gpio_ids[] = {
	{
		.name = "imx1-gpio",
		.driver_data = (unsigned long)&regs_imx1,
	}, {
		.name = "imx31-gpio",
		.driver_data = (unsigned long)&regs_imx31,
	}, {
		/* sentinel */
	},
};

static struct driver imx_gpio_driver = {
	.name = "imx-gpio",
	.probe = imx_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(imx_gpio_dt_ids),
	.id_table = imx_gpio_ids,
};

postcore_platform_driver(imx_gpio_driver);
