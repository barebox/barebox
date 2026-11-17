// SPDX-License-Identifier: GPL-2.0-only
/*
 * sama5d2 pin control and GPIO chip driver
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <pinctrl.h>
#include <malloc.h>
#include <gpio.h>
#include <mach/at91/gpio.h>
#include <linux/clk.h>
#include <linux/pinctrl/pinconf-generic.h>

#include <dt-bindings/pinctrl/at91.h>

#define ATMEL_GET_PIN_NO(pinfunc)	((pinfunc) & 0xff)
#define ATMEL_GET_PIN_FUNC(pinfunc)	((pinfunc >> 16) & 0xf)
#define ATMEL_GET_PIN_IOSET(pinfunc)	((pinfunc >> 20) & 0xf)

/* Custom pinconf parameters */
#define ATMEL_PIN_CONFIG_DRIVE_STRENGTH	(PIN_CONFIG_END + 1)

struct pinctrl_at91_pio4 {
	void __iomem *base;
	struct pinctrl_dev pinctrl;
	struct gpio_chip gpiochip;
};

struct at91_pinctrl_data {
        unsigned nbanks;
};

static const struct pinconf_generic_params atmel_custom_bindings[] = {
	{"atmel,drive-strength", ATMEL_PIN_CONFIG_DRIVE_STRENGTH, 0},
};

static inline void __iomem *pin_to_pio4(struct pinctrl_dev *pdev,
					unsigned int *pin)
{
	void __iomem *pio;
	struct pinctrl_at91_pio4 *pinctrl =
		container_of(pdev, struct pinctrl_at91_pio4, pinctrl);

	pio = pinctrl->base + (*pin / 32) * 0x40;
	*pin %= 32;

	return pio;
}

static void pinctrl_at91_pio4_update_config(struct pinctrl_dev *pdev,
					    unsigned int pin, u32 clear, u32 set)
{
	void __iomem *pio;
	u32 cfgr;

	pio = pin_to_pio4(pdev, &pin);

	writel(BIT(pin), pio + PIO4_MSKR);
	cfgr = readl(pio + PIO4_CFGR);
	cfgr &= ~clear;
	cfgr |= set;
	writel(cfgr, pio + PIO4_CFGR);
}

static int pinctrl_at91_pio4_pinmux_property_set(struct pinctrl_dev *pdev,
						 u32 pinmux_group)
{
	unsigned int pin = ATMEL_GET_PIN_NO(pinmux_group);
	unsigned int func = ATMEL_GET_PIN_FUNC(pinmux_group);

	pinctrl_at91_pio4_update_config(pdev, pin, PIO4_CFGR_FUNC_MASK,
					func & PIO4_CFGR_FUNC_MASK);

	return pin;
}

static int pinctrl_at91_pio4_set_config(struct pinctrl_dev *pdev,
					unsigned int pin, unsigned int param,
					u32 arg)
{
	unsigned int offset = pin;
	void __iomem *pio;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		pinctrl_at91_pio4_update_config(pdev, pin,
						PIO4_PUEN_MASK | PIO4_PDEN_MASK, 0);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_PDEN_MASK,
						PIO4_PUEN_MASK);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_PUEN_MASK,
						PIO4_PDEN_MASK);
		break;
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		pinctrl_at91_pio4_update_config(pdev, pin, 0, PIO4_OPD_MASK);
		break;
	case PIN_CONFIG_DRIVE_PUSH_PULL:
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_OPD_MASK, 0);
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		/* the bit disables the trigger, so it's set for arg == 0 */
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_SCHMITT_MASK,
						arg ? 0 : PIO4_SCHMITT_MASK);
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		if (!arg)
			return -ENOTSUPP;
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_DIR_MASK, 0);
		break;
	case PIN_CONFIG_OUTPUT_ENABLE:
		pinctrl_at91_pio4_update_config(pdev, pin, PIO4_DIR_MASK,
						arg ? PIO4_DIR_MASK : 0);
		break;
	case PIN_CONFIG_LEVEL:
		pio = pin_to_pio4(pdev, &offset);
		at91_mux_gpio4_set(pio, BIT(offset), arg);
		pinctrl_at91_pio4_update_config(pdev, pin, 0, PIO4_DIR_MASK);
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
	case ATMEL_PIN_CONFIG_DRIVE_STRENGTH:
		switch (arg) {
		case ATMEL_PIO_DRVSTR_LO:
		case ATMEL_PIO_DRVSTR_ME:
		case ATMEL_PIO_DRVSTR_HI:
			pinctrl_at91_pio4_update_config(pdev, pin,
							PIO4_DRVSTR_MASK,
							arg << PIO4_DRVSTR_OFFSET);
			break;
		default:
			dev_warn(pdev->dev, "drive strength not updated (incorrect value)\n");
		}
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int pinctrl_at91_pio4_pin_config_set(struct pinctrl_dev *pdev,
					    unsigned int pin,
					    unsigned long *configs,
					    unsigned int num_configs)
{
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);

		ret = pinctrl_at91_pio4_set_config(pdev, pin, param,
						   pinconf_to_config_argument(configs[i]));
		if (ret == -ENOTSUPP) {
			dev_warn(pdev->dev,
				 "unsupported configuration parameter: %u\n",
				 param);
			continue;
		}
		if (ret)
			return ret;
	}

	return 0;
}

static inline void __iomem *pin_to_gpio4(struct gpio_chip *chip, unsigned int *pin)
{
	void __iomem *gpio;
	struct pinctrl_at91_pio4 *pinctrl =
		container_of(chip, struct pinctrl_at91_pio4, gpiochip);

	gpio = pinctrl->base + (*pin / 32) * 0x40;
	*pin %= 32;

	return gpio;
}

static int at91_gpio4_direction_input(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_input(gpio, BIT(offset), true);

	return 0;
}

static int at91_gpio4_direction_output(struct gpio_chip *chip, unsigned offset,
				      int value)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_set(gpio, BIT(offset), value);
	at91_mux_gpio4_input(gpio, BIT(offset), false);

	return 0;
}

static int at91_gpio4_request(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_enable(gpio, BIT(offset));

	return 0;
}

static int at91_gpio4_get_direction(struct gpio_chip *chip,
				   unsigned int offset)
{
	u32 cfgr;
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	writel(BIT(offset), gpio + PIO4_MSKR);
	cfgr = readl(gpio + PIO4_CFGR);

	return cfgr & PIO4_DIR_MASK ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static int at91_gpio4_set(struct gpio_chip *chip, unsigned offset, int value)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_set(gpio, BIT(offset), value);

	return 0;
}

static int at91_gpio4_get(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	return at91_mux_gpio4_get(gpio, BIT(offset));
}

static struct gpio_ops at91_gpio4_ops = {
	.request = at91_gpio4_request,
	.direction_input = at91_gpio4_direction_input,
	.direction_output = at91_gpio4_direction_output,
	.get_direction = at91_gpio4_get_direction,
	.get = at91_gpio4_get,
	.set = at91_gpio4_set,
};

static int pinctrl_at91_pio4_gpiochip_add(struct device *dev,
					  struct pinctrl_at91_pio4 *pinctrl)
{
	const struct at91_pinctrl_data *drvdata;
	struct clk *clk;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(clk);
	if (ret < 0) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		clk_put(clk);
		return ret;
	}

	drvdata = device_get_match_data(dev);

	pinctrl->gpiochip.ops = &at91_gpio4_ops;
	pinctrl->gpiochip.base = 0;
	pinctrl->gpiochip.ngpio = drvdata->nbanks * MAX_NB_GPIO_PER_BANK;
	pinctrl->gpiochip.dev = dev;

	ret = gpiochip_add(&pinctrl->gpiochip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "gpio driver registered\n");

	return 0;
}

static const struct pinmux_ops pinctrl_at91_pio4_pmxops = {
	.pinmux_property_set = pinctrl_at91_pio4_pinmux_property_set,
};

static const struct pinconf_ops pinctrl_at91_pio4_confops = {
	.pin_config_set = pinctrl_at91_pio4_pin_config_set,
};

static const struct pinctrl_ops pinctrl_at91_pio4_ops = {
};

static const struct pinctrl_desc pinctrl_at91_pio4_desc = {
	.pctlops = &pinctrl_at91_pio4_ops,
	.pmxops = &pinctrl_at91_pio4_pmxops,
	.confops = &pinctrl_at91_pio4_confops,
	.custom_params = atmel_custom_bindings,
	.num_custom_params = ARRAY_SIZE(atmel_custom_bindings),
};

static int pinctrl_at91_pio4_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct pinctrl_at91_pio4 *pinctrl;
	struct resource *io;
	int ret;

	pinctrl = xzalloc(sizeof(*pinctrl));

	io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(io))
		return PTR_ERR(io);

	pinctrl->base = IOMEM(io->start);
	pinctrl->pinctrl.dev = dev;
	pinctrl->pinctrl.desc = &pinctrl_at91_pio4_desc;

	ret = pinctrl_register(&pinctrl->pinctrl);
	if (ret)
		return ret;

	dev_dbg(dev, "pinctrl driver registered\n");

	if (of_get_property(np, "gpio-controller", NULL))
		return pinctrl_at91_pio4_gpiochip_add(dev, pinctrl);

	return 0;
}

static const struct at91_pinctrl_data sama5d2_pinctrl_data = {
	.nbanks = 4,
};

static __maybe_unused struct of_device_id pinctrl_at91_pio4_dt_ids[] = {
	{ .compatible = "atmel,sama5d2-pinctrl", .data = &sama5d2_pinctrl_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pinctrl_at91_pio4_dt_ids);

static struct driver pinctrl_at91_pio4_driver = {
	.name		= "pinctrl-at91-pio4",
	.probe		= pinctrl_at91_pio4_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_at91_pio4_dt_ids),
};

core_platform_driver(pinctrl_at91_pio4_driver);
