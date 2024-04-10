// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2004 Tomas Marek, Elrest Solutions Company s.r.o.

/*
 * Based on the Linux kernel v6.8 drivers/pinctrl/intel/pinctrl-intel.c.
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>

#include <platform_data/gpio-intel.h>

#define PADBAR				0x00c

/* Offset from pad_regs */
#define PADCFG0				0x000
#define PADCFG0_RXEVCFG_MASK		GENMASK(26, 25)
#define PADCFG0_RXEVCFG_LEVEL		(0 << 25)
#define PADCFG0_RXEVCFG_EDGE		(1 << 25)
#define PADCFG0_RXEVCFG_DISABLED	(2 << 25)
#define PADCFG0_RXEVCFG_EDGE_BOTH	(3 << 25)
#define PADCFG0_PREGFRXSEL		BIT(24)
#define PADCFG0_RXINV			BIT(23)
#define PADCFG0_GPIROUTIOXAPIC		BIT(20)
#define PADCFG0_GPIROUTSCI		BIT(19)
#define PADCFG0_GPIROUTSMI		BIT(18)
#define PADCFG0_GPIROUTNMI		BIT(17)
#define PADCFG0_PMODE_SHIFT		10
#define PADCFG0_PMODE_MASK		GENMASK(13, 10)
#define PADCFG0_PMODE_GPIO		0
#define PADCFG0_GPIORXDIS		BIT(9)
#define PADCFG0_GPIOTXDIS		BIT(8)
#define PADCFG0_GPIORXSTATE		BIT(1)
#define PADCFG0_GPIOTXSTATE		BIT(0)

struct intel_gpio_chip {
	struct gpio_chip chip;
	void __iomem *community_pad_base;
};

static inline struct intel_gpio_chip *to_intel_gpio(struct gpio_chip *gc)
{
	return container_of(gc, struct intel_gpio_chip, chip);
}

static void __iomem *intel_gpio_padcfg0_reg(const struct intel_gpio_chip *chip,
					    const unsigned int gpio)
{
	const u32 pad_cfg_offset = 16;

	return chip->community_pad_base + pad_cfg_offset * gpio;
}

static u32 intel_gpio_padcfg0_value(const struct intel_gpio_chip *chip,
				    const unsigned int gpio)
{
	return readl(intel_gpio_padcfg0_reg(chip, gpio));
}

static void intel_gpio_padcfg0_write(const struct intel_gpio_chip *chip,
				     const unsigned int gpio, u32 value)
{
	writel(value, intel_gpio_padcfg0_reg(chip, gpio));
}

static void intel_gpio_set_value(struct gpio_chip *gc, unsigned int gpio,
				 int value)
{
	struct intel_gpio_chip *chip = to_intel_gpio(gc);
	u32 padcfg0;

	padcfg0 = intel_gpio_padcfg0_value(chip, gpio);
	if (value)
		padcfg0 |= PADCFG0_GPIOTXSTATE;
	else
		padcfg0 &= ~PADCFG0_GPIOTXSTATE;
	intel_gpio_padcfg0_write(chip, gpio, padcfg0);
}

static int intel_gpio_get_value(struct gpio_chip *gc, unsigned int gpio)
{
	struct intel_gpio_chip *chip = to_intel_gpio(gc);
	u32 padcfg0;

	padcfg0 = intel_gpio_padcfg0_value(chip, gpio);
	if (!(padcfg0 & PADCFG0_GPIOTXDIS))
		return !!(padcfg0 & PADCFG0_GPIOTXSTATE);

	return !!(padcfg0 & PADCFG0_GPIORXSTATE);
}

static int intel_gpio_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	struct intel_gpio_chip *chip = to_intel_gpio(gc);
	u32 padcfg0;

	padcfg0 = intel_gpio_padcfg0_value(chip, gpio);

	if (padcfg0 & PADCFG0_PMODE_MASK)
		return -EINVAL;

	if (padcfg0 & PADCFG0_GPIOTXDIS)
		return GPIOF_DIR_IN;

	return GPIOF_DIR_OUT;
}

static int intel_gpio_direction_input(struct gpio_chip *gc, unsigned int gpio)
{
	struct intel_gpio_chip *chip = to_intel_gpio(gc);
	u32 padcfg0;

	padcfg0 = intel_gpio_padcfg0_value(chip, gpio);
	padcfg0 &= ~PADCFG0_GPIORXDIS;
	padcfg0 |= PADCFG0_GPIOTXDIS;
	intel_gpio_padcfg0_write(chip, gpio, padcfg0);

	return 0;
}

static int intel_gpio_direction_output(struct gpio_chip *gc, unsigned int gpio,
				       int value)
{
	struct intel_gpio_chip *chip = to_intel_gpio(gc);
	u32 padcfg0;

	padcfg0 = intel_gpio_padcfg0_value(chip, gpio);
	padcfg0 &= ~PADCFG0_GPIOTXDIS;
	padcfg0 |= PADCFG0_GPIORXDIS;
	if (value)
		padcfg0 |= PADCFG0_GPIOTXSTATE;
	else
		padcfg0 &= ~PADCFG0_GPIOTXSTATE;
	intel_gpio_padcfg0_write(chip, gpio, padcfg0);

	return 0;
}

static struct gpio_ops intel_gpio_ops = {
	.direction_input = intel_gpio_direction_input,
	.direction_output = intel_gpio_direction_output,
	.get_direction = intel_gpio_get_direction,
	.get = intel_gpio_get_value,
	.set = intel_gpio_set_value,
};

static int intel_gpio_probe(struct device *dev)
{
	const struct gpio_intel_platform_data *pdata;
	struct intel_gpio_chip *intel_gpio;
	void __iomem *community_pad_base;
	void __iomem *community_base;
	struct resource *iores;
	int ret;

	pdata = (struct gpio_intel_platform_data *)dev->platform_data;
	if (!pdata) {
		dev_err(dev, "Configuration missing!\n");
		return -EINVAL;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "Memory resource request failed: %ld\n",
			PTR_ERR(iores));
		return PTR_ERR(iores);
	}

	intel_gpio = xzalloc(sizeof(*intel_gpio));

	intel_gpio->chip.ops = &intel_gpio_ops;
	intel_gpio->chip.ngpio = pdata->ngpios;
	intel_gpio->chip.dev = dev;

	community_base = IOMEM(iores->start);
	community_pad_base = IOMEM(community_base + readl(community_base + PADBAR));

	intel_gpio->community_pad_base = community_pad_base;

	ret = gpiochip_add(&intel_gpio->chip);

	if (ret) {
		dev_err(dev, "Couldn't add gpiochip: %pe\n", ERR_PTR(ret));
		kfree(intel_gpio);
		return ret;
	}

	return 0;
}

static struct driver_d intel_gpio_driver = {
	.name = "intel-gpio",
	.probe = intel_gpio_probe,
};

coredevice_platform_driver(intel_gpio_driver);
