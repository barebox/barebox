/*
 * Copyright (C) 2005 HP Labs
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 * Copyright (C) 2014 Raphaël Poggi
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <init.h>
#include <driver.h>
#include <getopt.h>

#include <mach/at91_pio.h>
#include <mach/gpio.h>
#include <mach/iomux.h>

#include <pinctrl.h>

struct at91_pinctrl {
	struct pinctrl_device pctl;
	struct at91_pinctrl_mux_ops	*ops;
};

struct at91_gpio_chip {
	struct gpio_chip	chip;
	void __iomem		*regbase;	/* PIO bank virtual address */
	struct at91_pinctrl_mux_ops *ops;	/* ops */
};

#define MAX_GPIO_BANKS		5
#define to_at91_pinctrl(c) container_of(c, struct at91_pinctrl, pctl);
#define to_at91_gpio_chip(c) container_of(c, struct at91_gpio_chip, chip)

#define PULL_UP         (1 << 0)
#define MULTI_DRIVE     (1 << 1)
#define DEGLITCH        (1 << 2)
#define PULL_DOWN       (1 << 3)
#define DIS_SCHMIT      (1 << 4)
#define DEBOUNCE        (1 << 16)
#define DEBOUNCE_VAL_SHIFT      17
#define DEBOUNCE_VAL    (0x3fff << DEBOUNCE_VAL_SHIFT)

static int gpio_banks;

static struct at91_gpio_chip gpio_chip[MAX_GPIO_BANKS];

static inline struct at91_gpio_chip *pin_to_controller(unsigned pin)
{
	pin /= MAX_NB_GPIO_PER_BANK;
	if (likely(pin < gpio_banks))
		return &gpio_chip[pin];

	return NULL;
}

/**
 * struct at91_pinctrl_mux_ops - describes an At91 mux ops group
 * on new IP with support for periph C and D the way to mux in
 * periph A and B has changed
 * So provide the right call back
 * if not present means the IP does not support it
 * @get_periph: return the periph mode configured
 * @mux_A_periph: mux as periph A
 * @mux_B_periph: mux as periph B
 * @mux_C_periph: mux as periph C
 * @mux_D_periph: mux as periph D
 * @set_deglitch: enable/disable deglitch
 * @set_debounce: enable/disable debounce
 * @set_pulldown: enable/disable pulldown
 * @disable_schmitt_trig: disable schmitt trigger
 */
struct at91_pinctrl_mux_ops {
	enum at91_mux (*get_periph)(void __iomem *pio, unsigned mask);
	void (*mux_A_periph)(void __iomem *pio, unsigned mask);
	void (*mux_B_periph)(void __iomem *pio, unsigned mask);
	void (*mux_C_periph)(void __iomem *pio, unsigned mask);
	void (*mux_D_periph)(void __iomem *pio, unsigned mask);
	bool (*get_deglitch)(void __iomem *pio, unsigned pin);
	void (*set_deglitch)(void __iomem *pio, unsigned mask, bool in_on);
	bool (*get_debounce)(void __iomem *pio, unsigned pin, u32 *div);
	void (*set_debounce)(void __iomem *pio, unsigned mask, bool in_on, u32 div);
	bool (*get_pulldown)(void __iomem *pio, unsigned pin);
	void (*set_pulldown)(void __iomem *pio, unsigned mask, bool in_on);
	bool (*get_schmitt_trig)(void __iomem *pio, unsigned pin);
	void (*disable_schmitt_trig)(void __iomem *pio, unsigned mask);
};

int at91_mux_pin(unsigned pin, enum at91_mux mux, int use_pullup)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem *pio;
	struct device_d *dev;
	unsigned mask = pin_to_mask(pin);
	int bank = pin_to_bank(pin);

	if (!at91_gpio)
		return -EINVAL;

	pio = at91_gpio->regbase;
	if (!pio)
		return -EINVAL;

	dev = at91_gpio->chip.dev;
	at91_mux_disable_interrupt(pio, mask);

	pin %= MAX_NB_GPIO_PER_BANK;
	if (mux) {
		dev_dbg(dev, "pio%c%d configured as periph%c with pullup = %d\n",
			bank + 'A', pin, mux - 1 + 'A', use_pullup);
	} else {
		dev_dbg(dev, "pio%c%d configured as gpio with pullup = %d\n",
			bank + 'A', pin, use_pullup);
	}

	switch(mux) {
	case AT91_MUX_GPIO:
		at91_mux_gpio_enable(pio, mask);
		break;
	case AT91_MUX_PERIPH_A:
		at91_gpio->ops->mux_A_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_B:
		at91_gpio->ops->mux_B_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_C:
		if (!at91_gpio->ops->mux_C_periph)
			return -EINVAL;
		at91_gpio->ops->mux_C_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_D:
		if (!at91_gpio->ops->mux_D_periph)
			return -EINVAL;
		at91_gpio->ops->mux_D_periph(pio, mask);
		break;
	default:
		return -EINVAL;
	}
	if (mux)
		at91_mux_gpio_disable(pio, mask);

	if (use_pullup >= 0)
		at91_mux_set_pullup(pio, mask, use_pullup);

	return 0;
}
EXPORT_SYMBOL(at91_mux_pin);

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral), and
 * configure it for an input.
 */
int at91_set_gpio_input(unsigned pin, int use_pullup)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);
	int ret;

	ret = at91_mux_pin(pin, AT91_MUX_GPIO, use_pullup);
	if (ret)
		return ret;

	dev_dbg(at91_gpio->chip.dev, "pio%c%d configured as input\n",
		pin_to_bank(pin) + 'A', pin_to_bank_offset(pin));

	at91_mux_gpio_input(pio, mask, true);

	return 0;
}

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral),
 * and configure it for an output.
 */
int at91_set_gpio_output(unsigned pin, int value)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);
	int ret;

	ret = at91_mux_pin(pin, AT91_MUX_GPIO, -1);
	if (ret)
		return ret;

	dev_dbg(at91_gpio->chip.dev, "pio%c%d configured as output val = %d\n",
		pin_to_bank(pin) + 'A', pin_to_bank_offset(pin), value);

	at91_mux_gpio_input(pio, mask, false);
	at91_mux_gpio_set(pio, mask, value);
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_output);

/*
 * enable/disable the glitch filter; mostly used with IRQ handling.
 */
int at91_set_deglitch(unsigned pin, int is_on)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	at91_gpio->ops->set_deglitch(pio, mask, is_on);
	return 0;
}
EXPORT_SYMBOL(at91_set_deglitch);

/*
 * enable/disable the debounce filter;
 */
int at91_set_debounce(unsigned pin, int is_on, int div)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !at91_gpio->ops->set_debounce)
		return -EINVAL;

	at91_gpio->ops->set_debounce(pio, mask, is_on, div);
	return 0;
}
EXPORT_SYMBOL(at91_set_debounce);

/*
 * enable/disable the multi-driver; This is only valid for output and
 * allows the output pin to run as an open collector output.
 */
int at91_set_multi_drive(unsigned pin, int is_on)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	at91_mux_set_multidrive(pio, mask, is_on);
	return 0;
}
EXPORT_SYMBOL(at91_set_multi_drive);

/*
 * enable/disable the pull-down.
 * If pull-up already enabled while calling the function, we disable it.
 */
int at91_set_pulldown(unsigned pin, int is_on)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !at91_gpio->ops->set_pulldown)
		return -EINVAL;

	/* Disable pull-up anyway */
	at91_mux_set_pullup(pio, mask, 0);
	at91_gpio->ops->set_pulldown(pio, mask, is_on);
	return 0;
}
EXPORT_SYMBOL(at91_set_pulldown);

/*
 * disable Schmitt trigger
 */
int at91_disable_schmitt_trig(unsigned pin)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !at91_gpio->ops->disable_schmitt_trig)
		return -EINVAL;

	at91_gpio->ops->disable_schmitt_trig(pio, mask);
	return 0;
}
EXPORT_SYMBOL(at91_disable_schmitt_trig);

static enum at91_mux at91_mux_pio3_get_periph(void __iomem *pio, unsigned mask)
{
	unsigned select;

	if (readl(pio + PIO_PSR) & mask)
		return AT91_MUX_GPIO;

	select = !!(readl(pio + PIO_ABCDSR1) & mask);
	select |= (!!(readl(pio + PIO_ABCDSR2) & mask) << 1);

	return select + 1;
}

static enum at91_mux at91_mux_get_periph(void __iomem *pio, unsigned mask)
{
	unsigned select;

	if (readl(pio + PIO_PSR) & mask)
		return AT91_MUX_GPIO;

	select = readl(pio + PIO_ABSR) & mask;

	return select + 1;
}

static bool at91_mux_get_deglitch(void __iomem *pio, unsigned pin)
{
	return (readl(pio + PIO_IFSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_debounce(void __iomem *pio, unsigned pin, u32 *div)
{
	*div = readl(pio + PIO_SCDR);

	return (readl(pio + PIO_IFSCSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_pulldown(void __iomem *pio, unsigned pin)
{
	return (readl(pio + PIO_PPDSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_schmitt_trig(void __iomem *pio, unsigned pin)
{
	return (readl(pio + PIO_SCHMITT) >> pin) & 0x1;
}

static struct at91_pinctrl_mux_ops at91rm9200_ops = {
	.get_periph	= at91_mux_get_periph,
	.mux_A_periph	= at91_mux_set_A_periph,
	.mux_B_periph	= at91_mux_set_B_periph,
	.get_deglitch	= at91_mux_get_deglitch,
	.set_deglitch	= at91_mux_set_deglitch,
};

static struct at91_pinctrl_mux_ops at91sam9x5_ops = {
	.get_periph	= at91_mux_pio3_get_periph,
	.mux_A_periph	= at91_mux_pio3_set_A_periph,
	.mux_B_periph	= at91_mux_pio3_set_B_periph,
	.mux_C_periph	= at91_mux_pio3_set_C_periph,
	.mux_D_periph	= at91_mux_pio3_set_D_periph,
	.get_deglitch	= at91_mux_get_deglitch,
	.set_deglitch	= at91_mux_pio3_set_deglitch,
	.get_debounce	= at91_mux_pio3_get_debounce,
	.set_debounce	= at91_mux_pio3_set_debounce,
	.get_pulldown	= at91_mux_pio3_get_pulldown,
	.set_pulldown	= at91_mux_pio3_set_pulldown,
	.get_schmitt_trig = at91_mux_pio3_get_schmitt_trig,
	.disable_schmitt_trig = at91_mux_pio3_disable_schmitt_trig,
};

static struct of_device_id at91_pinctrl_dt_ids[] = {
	{
		.compatible = "atmel,at91rm9200-pinctrl",
		.data = &at91rm9200_ops,
	}, {
		.compatible = "atmel,at91sam9x5-pinctrl",
		.data = &at91sam9x5_ops,
	}, {
		/* sentinel */
	}
};

static struct at91_pinctrl_mux_ops *at91_pinctrl_get_driver_data(struct device_d *dev)
{
	struct at91_pinctrl_mux_ops *ops_data = NULL;
	int rc;

	if (dev->device_node) {
		const struct of_device_id *match;
		match = of_match_node(at91_pinctrl_dt_ids, dev->device_node);
		if (!match)
			ops_data = NULL;
		else
			ops_data = (struct at91_pinctrl_mux_ops *)match->data;
	} else {
		rc = dev_get_drvdata(dev, (const void **)&ops_data);
		if (rc)
			ops_data = NULL;
	}

	return ops_data;
}

static int at91_pinctrl_set_conf(struct at91_pinctrl *info, unsigned int pin_num, unsigned int mux, unsigned int conf)
{
	unsigned int mask;
	void __iomem *pio;
	struct at91_gpio_chip *at91_gpio;

	at91_gpio = pin_to_controller(pin_num);
	pio  = at91_gpio->regbase;
	mask = pin_to_mask(pin_num);

	if (conf & PULL_UP && conf & PULL_DOWN)
		return -EINVAL;

	at91_mux_set_pullup(pio, mask, conf & PULL_UP);
	at91_mux_set_multidrive(pio, mask, conf & MULTI_DRIVE);
	if (info->ops->set_deglitch)
		info->ops->set_deglitch(pio, mask, conf & DEGLITCH);
	if (info->ops->set_debounce)
		info->ops->set_debounce(pio, mask, conf & DEBOUNCE,
			(conf & DEBOUNCE_VAL) >> DEBOUNCE_VAL_SHIFT);
	if (info->ops->set_pulldown)
		info->ops->set_pulldown(pio, mask, conf & PULL_DOWN);
	if (info->ops->disable_schmitt_trig && conf & DIS_SCHMIT)
		info->ops->disable_schmitt_trig(pio, mask);

	return 0;
}

static int at91_pinctrl_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct at91_pinctrl *info;
	const __be32 *list;
	int i, size;
	int ret = 0;
	int bank_num, pin_num, mux, conf;

	info = to_at91_pinctrl(pdev);

	list = of_get_property(np, "atmel,pins", &size);
	if (!list)
		return -EINVAL;

	size /= sizeof(*list);

	if (!size || size % 4) {
		dev_err(pdev->dev, "wrong pins number or pins and configs should be by 4\n");
		return -EINVAL;
	}

	for (i = 0; i < size; i += 4) {
		bank_num = be32_to_cpu(*list++);
		pin_num = be32_to_cpu(*list++);
		mux = be32_to_cpu(*list++);
		conf = be32_to_cpu(*list++);

		pin_num += bank_num * MAX_NB_GPIO_PER_BANK;

		ret = at91_mux_pin(pin_num, mux, conf & PULL_UP);
		if (ret) {
			dev_err(pdev->dev, "failed to mux pin %d\n", pin_num);
			return ret;
		}

		ret = at91_pinctrl_set_conf(info, pin_num, mux, conf);
		if (ret) {
			dev_err(pdev->dev, "failed to set conf on pin %d\n", pin_num);
			return ret;
		}
	}

	return ret;
}

static struct pinctrl_ops at91_pinctrl_ops = {
	.set_state = at91_pinctrl_set_state,
};

static int at91_pinctrl_probe(struct device_d *dev)
{
	struct at91_pinctrl *info;
	int ret;

	if (!IS_ENABLED(CONFIG_PINCTRL))
		return 0;

	info = xzalloc(sizeof(struct at91_pinctrl));

	info->ops = at91_pinctrl_get_driver_data(dev);
	if (!info->ops) {
		dev_err(dev, "failed to retrieve driver data\n");
		return -ENODEV;
	}

	info->pctl.dev = dev;
	info->pctl.ops = &at91_pinctrl_ops;

	ret = pinctrl_register(&info->pctl);
	if (ret)
		return ret;

	dev_dbg(dev, "AT91 pinctrl registered\n");

	return 0;
}

static struct platform_device_id at91_pinctrl_ids[] = {
	{
		.name = "at91rm9200-pinctrl",
		.driver_data = (unsigned long)&at91rm9200_ops,
	}, {
		.name = "at91sam9x5-pinctrl",
		.driver_data = (unsigned long)&at91sam9x5_ops,
	}, {
		/* sentinel */
	},
};

static struct driver_d at91_pinctrl_driver = {
	.name = "pinctrl-at91",
	.probe = at91_pinctrl_probe,
	.id_table = at91_pinctrl_ids,
	.of_compatible = DRV_OF_COMPAT(at91_pinctrl_dt_ids),
};

static int at91_pinctrl_init(void)
{
	return platform_driver_register(&at91_pinctrl_driver);
}
core_initcall(at91_pinctrl_init);

static int at91_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	return at91_mux_gpio_get(pio, mask);
}

static void at91_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	at91_mux_gpio_set(pio, mask, value);
}

static int at91_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
		int value)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	at91_mux_gpio_set(pio, mask, value);
	writel(mask, pio + PIO_OER);

	return 0;
}

static int at91_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;
	u32 osr;

	if (mask & readl(pio + PIO_PSR)) {
		osr = readl(pio + PIO_OSR);
		return !(osr & mask);
	} else {
		return -EBUSY;
	}
}

static int at91_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	writel(mask, pio + PIO_ODR);

	return 0;
}

static int at91_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	dev_dbg(chip->dev, "%s:%d pio%c%d(%d)\n", __func__, __LINE__,
		 'A' + pin_to_bank(chip->base), offset, chip->base + offset);
	at91_mux_gpio_enable(pio, mask);

	return 0;
}

static void at91_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	dev_dbg(chip->dev, "%s:%d pio%c%d(%d)\n", __func__, __LINE__,
		 'A' + pin_to_bank(chip->base), offset, chip->base + offset);
}

static struct gpio_ops at91_gpio_ops = {
	.request = at91_gpio_request,
	.free = at91_gpio_free,
	.direction_input = at91_gpio_direction_input,
	.direction_output = at91_gpio_direction_output,
	.get_direction = at91_gpio_get_direction,
	.get = at91_gpio_get,
	.set = at91_gpio_set,
};

static struct of_device_id at91_gpio_dt_ids[] = {
	{
		.compatible = "atmel,at91rm9200-gpio",
		.data = &at91rm9200_ops,
	}, {
		.compatible = "atmel,at91sam9x5-gpio",
		.data = &at91sam9x5_ops,
	}, {
		/* sentinel */
	},
};

static int at91_gpio_probe(struct device_d *dev)
{
	struct at91_gpio_chip *at91_gpio;
	struct clk *clk;
	int ret;
	int alias_idx;

	if (dev->device_node)
		alias_idx = of_alias_get_id(dev->device_node, "gpio");
	else
		alias_idx = dev->id;

	BUG_ON(alias_idx > MAX_GPIO_BANKS);

	at91_gpio = &gpio_chip[alias_idx];

	ret = dev_get_drvdata(dev, (const void **)&at91_gpio->ops);
        if (ret) {
                dev_err(dev, "dev_get_drvdata failed: %d\n", ret);
                return ret;
        }

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

	gpio_banks = max(gpio_banks, alias_idx + 1);
	at91_gpio->regbase = dev_request_mem_region_err_null(dev, 0);
	if (!at91_gpio->regbase)
		return -ENOENT;

	at91_gpio->chip.ops = &at91_gpio_ops;
	at91_gpio->chip.ngpio = MAX_NB_GPIO_PER_BANK;
	at91_gpio->chip.dev = dev;
	at91_gpio->chip.base = alias_idx * MAX_NB_GPIO_PER_BANK;

	ret = gpiochip_add(&at91_gpio->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "AT91 gpio driver registered\n");

	return 0;
}

static struct platform_device_id at91_gpio_ids[] = {
	{
		.name = "at91rm9200-gpio",
                .driver_data = (unsigned long)&at91rm9200_ops,
	}, {
		.name = "at91sam9x5-gpio",
		.driver_data = (unsigned long)&at91sam9x5_ops,
	}, {
		/* sentinel */
	},
};

static struct driver_d at91_gpio_driver = {
	.name = "gpio-at91",
	.probe = at91_gpio_probe,
	.id_table = at91_gpio_ids,
	.of_compatible	= DRV_OF_COMPAT(at91_gpio_dt_ids),
};

static int at91_gpio_init(void)
{
	return platform_driver_register(&at91_gpio_driver);
}
core_initcall(at91_gpio_init);
