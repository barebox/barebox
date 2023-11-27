// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 Maxime Coquelin
 * Copyright (C) 2017 STMicroelectronics
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <pinctrl.h>
#include <gpio.h>
#include <hwspinlock.h>
#include <malloc.h>
#include <linux/clk.h>
#include <soc/stm32/gpio.h>

#define STM32_PIN_NO(x) ((x) << 8)
#define STM32_GET_PIN_NO(x) ((x) >> 8)
#define STM32_GET_PIN_FUNC(x) ((x) & 0xff)

struct stm32_gpio_bank {
	void __iomem *base;
	struct gpio_chip chip;
	const char *name;
};

struct stm32_pinctrl {
	struct pinctrl_device pdev;
	struct hwspinlock hws;
	struct stm32_gpio_bank gpio_banks[];
};

static inline struct stm32_pinctrl *to_stm32_pinctrl(struct pinctrl_device *pdev)
{
	return container_of(pdev, struct stm32_pinctrl, pdev);
}

static inline struct stm32_gpio_bank *to_stm32_gpio_bank(struct gpio_chip *chip)
{
	return container_of(chip, struct stm32_gpio_bank, chip);
}

static inline int stm32_gpio_pin(int gpio, struct stm32_gpio_bank **bank)
{
	if (bank) {
		struct gpio_chip *chip;

		chip = gpio_get_chip(gpio);
		if (!chip)
			return -EINVAL;

		*bank = to_stm32_gpio_bank(chip);
	}

	return gpio % STM32_GPIO_PINS_PER_BANK;
}

static inline u32 stm32_gpio_get_mode(u32 function)
{
	switch (function) {
	case STM32_PIN_GPIO:
		return STM32_PINMODE_GPIO;
	case STM32_PIN_AF(0) ... STM32_PIN_AF(15):
		return STM32_PINMODE_AF;
	case STM32_PIN_ANALOG:
		return STM32_PINMODE_ANALOG;
	}

	return 0;
}

static inline u32 stm32_gpio_get_alt(u32 function)
{
	switch (function) {
	case STM32_PIN_GPIO:
		return 0;
	case STM32_PIN_AF(0) ... STM32_PIN_AF(15):
		return function - 1;
	case STM32_PIN_ANALOG:
		return 0;
	}

	return 0;
}

static int __stm32_pinctrl_set_state(struct device *dev,
				     struct device_node *pins)
{
	int ret;

	int num_pins = 0, i;
	u32 slew_rate;
	bool adjust_slew_rate = false;
	enum stm32_pin_bias bias = -1;
	enum stm32_pin_out_type out_type = -1;
	enum { PIN_INPUT, PIN_OUTPUT_LOW, PIN_OUTPUT_HIGH } dir = -1;

	of_get_property(pins, "pinmux", &num_pins);
	num_pins /= sizeof(__be32);
	if (!num_pins) {
		dev_err(dev, "Invalid pinmux property in %pOF\n", pins);
		return -EINVAL;
	}

	ret = of_property_read_u32(pins, "slew-rate", &slew_rate);
	if (!ret)
		adjust_slew_rate = true;

	if (of_get_property(pins, "bias-disable", NULL))
		bias = STM32_PIN_NO_BIAS;
	else if (of_get_property(pins, "bias-pull-up", NULL))
		bias = STM32_PIN_PULL_UP;
	else if (of_get_property(pins, "bias-pull-down", NULL))
		bias = STM32_PIN_PULL_DOWN;

	if (of_get_property(pins, "drive-push-pull", NULL))
		out_type = STM32_PIN_OUT_PUSHPULL;
	else if (of_get_property(pins, "drive-open-drain", NULL))
		out_type = STM32_PIN_OUT_OPENDRAIN;

	if (of_get_property(pins, "input-enable", NULL))
		dir = PIN_INPUT;
	else if (of_get_property(pins, "output-low", NULL))
		dir = PIN_OUTPUT_LOW;
	else if (of_get_property(pins, "output-high", NULL))
		dir = PIN_OUTPUT_HIGH;

	dev_dbg(dev, "%pOF: multiplexing %d pins\n", pins, num_pins);

	for (i = 0; i < num_pins; i++) {
		struct stm32_gpio_bank *bank = NULL;
		u32 pinfunc, mode, alt;
		unsigned func;
		int offset;

		ret = of_property_read_u32_index(pins, "pinmux",
				i, &pinfunc);
		if (ret)
			return ret;

		func = STM32_GET_PIN_FUNC(pinfunc);
		offset = stm32_gpio_pin(STM32_GET_PIN_NO(pinfunc), &bank);
		if (offset < 0)
			return -ENODEV;

		mode = stm32_gpio_get_mode(func);
		alt = stm32_gpio_get_alt(func);

		dev_dbg(dev, "configuring port %s pin %u with:\n\t"
			"fn %u, mode %u, alt %u\n",
			bank->name, offset, func, mode, alt);

		__stm32_pmx_set_mode(bank->base, offset, mode, alt);

		if (adjust_slew_rate)
			__stm32_pmx_set_speed(bank->base, offset, slew_rate);

		if (bias != -1)
			__stm32_pmx_set_bias(bank->base, offset, bias);

		if (out_type != -1)
			__stm32_pmx_set_output_type(bank->base, offset, out_type);

		if (dir == PIN_INPUT)
			__stm32_pmx_gpio_input(bank->base, offset);
		else if (dir == PIN_OUTPUT_LOW)
			__stm32_pmx_gpio_output(bank->base, offset, 0);
		else if (dir == PIN_OUTPUT_HIGH)
			__stm32_pmx_gpio_output(bank->base, offset, 1);
	}

	return 0;
}

static int stm32_pinctrl_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct stm32_pinctrl *pinctrl = to_stm32_pinctrl(pdev);
	struct device *dev = pdev->dev;
	struct device_node *pins;
	void *prop;
	int ret;

	ret = hwspinlock_lock_timeout(&pinctrl->hws, 10);
	if (ret == -ETIMEDOUT) {
		dev_err(dev, "hw spinlock timeout\n");
		return ret;
	}

	prop = of_find_property(np, "pinmux", NULL);
	if (prop) {
		ret = __stm32_pinctrl_set_state(dev, np);
		goto out;
	}

	for_each_child_of_node(np, pins) {
		ret = __stm32_pinctrl_set_state(dev, pins);
		if (ret)
			goto out;
	}

out:
	hwspinlock_unlock(&pinctrl->hws);
	return ret;
}

/* GPIO functions */

static int stm32_gpio_get_direction(struct gpio_chip *chip, unsigned int gpio)
{
	struct stm32_gpio_bank *bank = to_stm32_gpio_bank(chip);
	int ret;
	u32 mode, alt;

	__stm32_pmx_get_mode(bank->base, stm32_gpio_pin(gpio, NULL), &mode, &alt);
	if ((alt == 0) && (mode == 0))
		ret = 1;
	else if ((alt == 0) && (mode == 1))
		ret = 0;
	else
		ret = -EINVAL;

	return ret;
}

static void stm32_gpio_set(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct stm32_gpio_bank *bank = to_stm32_gpio_bank(chip);

	__stm32_pmx_gpio_set(bank->base, stm32_gpio_pin(gpio, NULL), value);
}

static int stm32_gpio_get(struct gpio_chip *chip, unsigned gpio)
{
	struct stm32_gpio_bank *bank = to_stm32_gpio_bank(chip);

	return __stm32_pmx_gpio_get(bank->base, stm32_gpio_pin(gpio, NULL));
}

static int stm32_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct stm32_gpio_bank *bank = to_stm32_gpio_bank(chip);

	__stm32_pmx_gpio_input(bank->base, stm32_gpio_pin(gpio, NULL));

	return 0;
}

static int stm32_gpio_direction_output(struct gpio_chip *chip,
				       unsigned gpio, int value)
{
	struct stm32_gpio_bank *bank = to_stm32_gpio_bank(chip);

	__stm32_pmx_gpio_output(bank->base, stm32_gpio_pin(gpio, NULL), value);

	return 0;
}

static struct gpio_ops stm32_gpio_ops = {
	.direction_input = stm32_gpio_direction_input,
	.direction_output = stm32_gpio_direction_output,
	.get_direction = stm32_gpio_get_direction,
	.get = stm32_gpio_get,
	.set = stm32_gpio_set,
};

static int stm32_gpiochip_add(struct stm32_gpio_bank *bank,
			      struct device_node *np,
			      struct device *parent)
{
	struct device *dev;
	struct resource *iores;
	enum { PINCTRL_PHANDLE, GPIOCTRL_OFFSET, PINCTRL_OFFSET, PINCOUNT, GPIO_RANGE_NCELLS };
	const __be32 *gpio_ranges;
	struct clk *clk;
	u32 ngpios;
	int ret, size;

	dev = of_platform_device_create(np, parent);
	if (!dev)
		return -ENODEV;

	gpio_ranges = of_get_property(np, "gpio-ranges", &size);
	size /= sizeof(__be32);
	if (!gpio_ranges || size < GPIO_RANGE_NCELLS) {
		dev_err(dev, "Couldn't read 'gpio-ranges' property in %pOF\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "ngpios", &ngpios);
	if (ret)
		ngpios = be32_to_cpu(gpio_ranges[PINCOUNT]);

	bank->chip.ngpio = ngpios;

	if (size > GPIO_RANGE_NCELLS) {
		dev_err(dev, "Unsupported disjunct 'gpio-ranges' in %pOF\n", np);
		return -EINVAL;
	}

	if (ngpios > STM32_GPIO_PINS_PER_BANK) {
		dev_err(dev, "ngpios property expected to be %u at most in %pOF\n",
			ngpios, np);
		return -EINVAL;
	}

	ret = of_property_read_string(np, "st,bank-name", &bank->name);
	if (ret)
		bank->name = np->name;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "Failed to request GPIO memory resource\n");
		return PTR_ERR(iores);
	}

	bank->base = IOMEM(iores->start);

	bank->chip.base = be32_to_cpu(gpio_ranges[PINCTRL_OFFSET]);
	bank->chip.ops  = &stm32_gpio_ops;
	bank->chip.dev  = dev;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(dev, "failed to get clk (%ld)\n", PTR_ERR(clk));
		return PTR_ERR(clk);
	}

	clk_enable(clk);

	return gpiochip_add(&bank->chip);
}

static struct pinctrl_ops stm32_pinctrl_ops = {
	.set_state = stm32_pinctrl_set_state,
};

static int stm32_pinctrl_probe(struct device *dev)
{
	struct stm32_pinctrl *pinctrl;
	unsigned nbanks = 0;
	struct stm32_gpio_bank *gpio_bank;
	struct device_node *np = dev->of_node, *child;
	int ret;

	for_each_available_child_of_node(np, child)
		if (of_property_read_bool(child, "gpio-controller"))
			nbanks++;

	pinctrl = xzalloc(sizeof(*pinctrl) + nbanks * sizeof(struct stm32_gpio_bank));

	pinctrl->pdev.dev = dev;
	pinctrl->pdev.ops = &stm32_pinctrl_ops;

	/* hwspinlock property is optional, just log the error */
	ret = hwspinlock_get_by_index(dev, 0, &pinctrl->hws);
	if (ret)
		dev_dbg(dev, "proceeding without hw spinlock support: (%d)\n",
			ret);

	gpio_bank = pinctrl->gpio_banks;
	for_each_available_child_of_node(np, child) {
		if (!of_property_read_bool(child, "gpio-controller"))
			continue;

		ret = stm32_gpiochip_add(gpio_bank, child, dev);
		if (ret) {
			dev_err(dev, "couldn't add gpiochip %s, ret = %d\n", child->name, ret);
			return ret;
		}

		of_platform_device_dummy_drv(gpio_bank->chip.dev);
		gpio_bank++;
	}

	return pinctrl_register(&pinctrl->pdev);
}

static __maybe_unused struct of_device_id stm32_pinctrl_dt_ids[] = {
	{ .compatible = "st,stm32f429-pinctrl" },
	{ .compatible = "st,stm32f469-pinctrl" },
	{ .compatible = "st,stm32f746-pinctrl" },
	{ .compatible = "st,stm32h743-pinctrl" },
	{ .compatible = "st,stm32mp157-pinctrl" },
	{ .compatible = "st,stm32mp157-z-pinctrl" },
	{ .compatible = "st,stm32mp135-pinctrl" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stm32_pinctrl_dt_ids);

static struct driver stm32_pinctrl_driver = {
	.name		= "stm32-pinctrl",
	.probe		= stm32_pinctrl_probe,
	.of_compatible	= DRV_OF_COMPAT(stm32_pinctrl_dt_ids),
};

core_platform_driver(stm32_pinctrl_driver);
