// SPDX-License-Identifier: GPL-2.0-only
/*
 * GPIO driver for the Intel/Marvell PXA SoCs
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of_device.h>

/*
 * The registers are grouped in banks of 32 GPIOs. The first three banks
 * are interleaved, one 32bit register per bank, and every further group
 * of three banks starts at the next 0x100 boundary.
 */
#define GPLR	0x00	/* pin level, read only */
#define GPDR	0x0c	/* direction, 1 = output */
#define GPSR	0x18	/* set output, write 1 to set */
#define GPCR	0x24	/* clear output, write 1 to clear */

#define BANK_OFF(bank)	((((bank) / 3) << 8) + (((bank) % 3) << 2))

struct pxa_gpio_chip {
	struct gpio_chip chip;
	void __iomem *base;
};

static inline struct pxa_gpio_chip *to_pxa_gpio(struct gpio_chip *chip)
{
	return container_of(chip, struct pxa_gpio_chip, chip);
}

static inline void __iomem *pxa_gpio_reg(struct gpio_chip *chip,
					 unsigned off, unsigned reg)
{
	return to_pxa_gpio(chip)->base + BANK_OFF(off / 32) + reg;
}

static int pxa_gpio_get(struct gpio_chip *chip, unsigned off)
{
	return !!(readl(pxa_gpio_reg(chip, off, GPLR)) & BIT(off % 32));
}

static int pxa_gpio_set(struct gpio_chip *chip, unsigned off, int value)
{
	writel(BIT(off % 32), pxa_gpio_reg(chip, off, value ? GPSR : GPCR));

	return 0;
}

static int pxa_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	void __iomem *gpdr = pxa_gpio_reg(chip, off, GPDR);

	writel(readl(gpdr) & ~BIT(off % 32), gpdr);

	return 0;
}

static int pxa_gpio_direction_output(struct gpio_chip *chip, unsigned off,
				     int value)
{
	void __iomem *gpdr = pxa_gpio_reg(chip, off, GPDR);

	/* drive the requested level before switching the pin to output */
	pxa_gpio_set(chip, off, value);
	writel(readl(gpdr) | BIT(off % 32), gpdr);

	return 0;
}

static int pxa_gpio_get_direction(struct gpio_chip *chip, unsigned off)
{
	if (readl(pxa_gpio_reg(chip, off, GPDR)) & BIT(off % 32))
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static struct gpio_ops pxa_gpio_ops = {
	.direction_input = pxa_gpio_direction_input,
	.direction_output = pxa_gpio_direction_output,
	.get_direction = pxa_gpio_get_direction,
	.get = pxa_gpio_get,
	.set = pxa_gpio_set,
};

static int pxa_gpio_probe(struct device *dev)
{
	struct pxa_gpio_chip *pxa;
	struct resource *iores;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	pxa = xzalloc(sizeof(*pxa));
	pxa->base = IOMEM(iores->start);

	pxa->chip.dev = dev;
	pxa->chip.ops = &pxa_gpio_ops;
	pxa->chip.base = 0;
	pxa->chip.ngpio = (uintptr_t)device_get_match_data(dev);

	ret = gpiochip_add(&pxa->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip: %pe\n", ERR_PTR(ret));
		free(pxa);
		return ret;
	}

	dev_dbg(dev, "probed %u gpios\n", pxa->chip.ngpio);

	return 0;
}

static struct of_device_id pxa_gpio_dt_ids[] = {
	{
		.compatible = "intel,pxa25x-gpio",
		.data = (void *)85,
	}, {
		.compatible = "intel,pxa26x-gpio",
		.data = (void *)90,
	}, {
		.compatible = "intel,pxa27x-gpio",
		.data = (void *)121,
	}, {
		.compatible = "intel,pxa3xx-gpio",
		.data = (void *)128,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, pxa_gpio_dt_ids);

static struct driver pxa_gpio_driver = {
	.name = "pxa-gpio",
	.probe = pxa_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(pxa_gpio_dt_ids),
};
core_platform_driver(pxa_gpio_driver);
