/*
 * Copyright (C) 2005 HP Labs
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <linux/clk.h>
#include <linux/err.h>
#include <errno.h>
#include <io.h>
#include <mach/gpio.h>
#include <mach/io.h>
#include <mach/cpu.h>
#include <gpio.h>
#include <init.h>
#include <driver.h>

#define MAX_GPIO_BANKS		5
#define MAX_NB_GPIO_PER_BANK	32

static int gpio_banks = 0;

/*
 * Functionnality can change with newer chips
 */
struct at91_gpio_chip {
	struct device_d		*dev;
	void __iomem		*regbase;	/* PIO bank virtual address */
	struct at91_pinctrl_mux_ops *ops;	/* ops */
};

static struct at91_gpio_chip gpio_chip[MAX_GPIO_BANKS];

static inline unsigned pin_to_bank(unsigned pin)
{
	return pin / MAX_NB_GPIO_PER_BANK;
}

static inline unsigned pin_to_bank_offset(unsigned pin)
{
	return pin % MAX_NB_GPIO_PER_BANK;
}

static inline struct at91_gpio_chip *pin_to_controller(unsigned pin)
{
	pin /= MAX_NB_GPIO_PER_BANK;
	if (likely(pin < gpio_banks))
		return &gpio_chip[pin];

	return NULL;
}

static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << pin_to_bank_offset(pin);
}

/**
 * struct at91_pinctrl_mux_ops - describes an At91 mux ops group
 * on new IP with support for periph C and D the way to mux in
 * periph A and B has changed
 * So provide the right call back
 * if not present means the IP does not support it
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
	void (*mux_A_periph)(void __iomem *pio, unsigned mask);
	void (*mux_B_periph)(void __iomem *pio, unsigned mask);
	void (*mux_C_periph)(void __iomem *pio, unsigned mask);
	void (*mux_D_periph)(void __iomem *pio, unsigned mask);
	void (*set_deglitch)(void __iomem *pio, unsigned mask, bool in_on);
	void (*set_debounce)(void __iomem *pio, unsigned mask, bool in_on, u32 div);
	void (*set_pulldown)(void __iomem *pio, unsigned mask, bool in_on);
	void (*disable_schmitt_trig)(void __iomem *pio, unsigned mask);
};

static void at91_mux_disable_interrupt(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_IDR);
}

static void at91_mux_set_pullup(void __iomem *pio, unsigned mask, bool on)
{
	__raw_writel(mask, pio + (on ? PIO_PUER : PIO_PUDR));
}

static void at91_mux_set_multidrive(void __iomem *pio, unsigned mask, bool on)
{
	__raw_writel(mask, pio + (on ? PIO_MDER : PIO_MDDR));
}

static void at91_mux_set_A_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_ASR);
}

static void at91_mux_set_B_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_BSR);
}

static void at91_mux_pio3_set_A_periph(void __iomem *pio, unsigned mask)
{

	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask,
						pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
						pio + PIO_ABCDSR2);
}

static void at91_mux_pio3_set_B_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask,
						pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
						pio + PIO_ABCDSR2);
}

static void at91_mux_pio3_set_C_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
}

static void at91_mux_pio3_set_D_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
}

static void at91_mux_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	__raw_writel(mask, pio + (is_on ? PIO_IFER : PIO_IFDR));
}

static void at91_mux_pio3_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	if (is_on)
		__raw_writel(mask, pio + PIO_IFSCDR);
	at91_mux_set_deglitch(pio, mask, is_on);
}

static void at91_mux_pio3_set_debounce(void __iomem *pio, unsigned mask,
				bool is_on, u32 div)
{
	if (is_on) {
		__raw_writel(mask, pio + PIO_IFSCER);
		__raw_writel(div & PIO_SCDR_DIV, pio + PIO_SCDR);
		__raw_writel(mask, pio + PIO_IFER);
	} else {
		__raw_writel(mask, pio + PIO_IFDR);
	}
}

static void at91_mux_pio3_set_pulldown(void __iomem *pio, unsigned mask, bool is_on)
{
	__raw_writel(mask, pio + (is_on ? PIO_PPDER : PIO_PPDDR));
}

static void at91_mux_pio3_disable_schmitt_trig(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_SCHMITT) | mask, pio + PIO_SCHMITT);
}

static struct at91_pinctrl_mux_ops at91rm9200_ops = {
	.mux_A_periph	= at91_mux_set_A_periph,
	.mux_B_periph	= at91_mux_set_B_periph,
	.set_deglitch	= at91_mux_set_deglitch,
};

static struct at91_pinctrl_mux_ops at91sam9x5_ops = {
	.mux_A_periph	= at91_mux_pio3_set_A_periph,
	.mux_B_periph	= at91_mux_pio3_set_B_periph,
	.mux_C_periph	= at91_mux_pio3_set_C_periph,
	.mux_D_periph	= at91_mux_pio3_set_D_periph,
	.set_deglitch	= at91_mux_pio3_set_deglitch,
	.set_debounce	= at91_mux_pio3_set_debounce,
	.set_pulldown	= at91_mux_pio3_set_pulldown,
	.disable_schmitt_trig = at91_mux_pio3_disable_schmitt_trig,
};

static void at91_mux_gpio_disable(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_PDR);
}

static void at91_mux_gpio_enable(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_PER);
}

static void at91_mux_gpio_input(void __iomem *pio, unsigned mask, bool input)
{
	__raw_writel(mask, pio + (input ? PIO_ODR : PIO_OER));
}

int at91_mux_pin(unsigned pin, enum at91_mux mux, int use_pullup)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = pin_to_mask(pin);
	int bank = pin_to_bank(pin);
	struct device_d *dev = at91_gpio->dev;

	if (!at91_gpio)
		return -EINVAL;

	pio = at91_gpio->regbase;
	if (!pio)
		return -EINVAL;

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

	dev_dbg(at91_gpio->dev, "pio%c%d configured as input\n",
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

	dev_dbg(at91_gpio->dev, "pio%c%d configured as output val = %d\n",
		pin_to_bank(pin) + 'A', pin_to_bank_offset(pin), value);

	at91_mux_gpio_input(pio, mask, false);
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
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

/*
 * assuming the pin is muxed as a gpio output, set its value.
 */
int at91_set_gpio_value(unsigned pin, int value)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_value);

/*
 * read the pin's value (works even if it's not muxed as a gpio).
 */
int at91_get_gpio_value(unsigned pin)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);
	u32		pdsr;

	if (!pio)
		return -EINVAL;
	pdsr = __raw_readl(pio + PIO_PDSR);
	return (pdsr & mask) != 0;
}
EXPORT_SYMBOL(at91_get_gpio_value);

int gpio_direction_input(unsigned pin)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(__raw_readl(pio + PIO_PSR) & mask))
		return -EINVAL;
	at91_mux_gpio_input(pio, mask, true);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_input);

int gpio_direction_output(unsigned pin, int value)
{
	struct at91_gpio_chip *at91_gpio = pin_to_controller(pin);
	void __iomem	*pio = at91_gpio->regbase;
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(__raw_readl(pio + PIO_PSR) & mask))
		return -EINVAL;
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	at91_mux_gpio_input(pio, mask, false);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_output);

/*--------------------------------------------------------------------------*/

static int at91_gpio_probe(struct device_d *dev)
{
	struct at91_gpio_chip *at91_gpio;
	struct clk *clk;
	int ret;

	BUG_ON(dev->id > MAX_GPIO_BANKS);

	at91_gpio = &gpio_chip[dev->id];

	ret = dev_get_drvdata(dev, (unsigned long *)&at91_gpio->ops);
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

	at91_gpio->dev = dev;
	gpio_banks = max(gpio_banks, dev->id + 1);
	at91_gpio->regbase = dev_request_mem_region(dev, 0);

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
	.name = "at91-gpio",
	.probe = at91_gpio_probe,
	.id_table = at91_gpio_ids,
};

static int at91_gpio_init(void)
{
	return platform_driver_register(&at91_gpio_driver);
}
postcore_initcall(at91_gpio_init);
