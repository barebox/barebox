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
#include <command.h>
#include <complete.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <errno.h>
#include <io.h>
#include <mach/iomux.h>
#include <mach/io.h>
#include <mach/cpu.h>
#include <gpio.h>
#include <init.h>
#include <driver.h>
#include <getopt.h>

#include "gpio.h"

#define MAX_GPIO_BANKS		5

static int gpio_banks = 0;

/*
 * Functionnality can change with newer chips
 */
struct at91_gpio_chip {
	struct gpio_chip	chip;
	void __iomem		*regbase;	/* PIO bank virtual address */
	struct at91_pinctrl_mux_ops *ops;	/* ops */
};

#define to_at91_gpio_chip(c) container_of(c, struct at91_gpio_chip, chip)

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

#ifdef CONFIG_CMD_AT91MUX
static unsigned at91_mux_get_pullup(void __iomem *pio, unsigned pin)
{
	return (__raw_readl(pio + PIO_PUSR) >> pin) & 0x1;
}

static unsigned at91_mux_get_multidrive(void __iomem *pio, unsigned pin)
{
	return (__raw_readl(pio + PIO_MDSR) >> pin) & 0x1;
}

static enum at91_mux at91_mux_pio3_get_periph(void __iomem *pio, unsigned mask)
{
	unsigned select;

	if (__raw_readl(pio + PIO_PSR) & mask)
		return AT91_MUX_GPIO;

	select = !!(__raw_readl(pio + PIO_ABCDSR1) & mask);
	select |= (!!(__raw_readl(pio + PIO_ABCDSR2) & mask) << 1);

	return select + 1;
}

static enum at91_mux at91_mux_get_periph(void __iomem *pio, unsigned mask)
{
	unsigned select;

	if (__raw_readl(pio + PIO_PSR) & mask)
		return AT91_MUX_GPIO;

	select = __raw_readl(pio + PIO_ABSR) & mask;

	return select + 1;
}

static bool at91_mux_get_deglitch(void __iomem *pio, unsigned pin)
{
	return (__raw_readl(pio + PIO_IFSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_debounce(void __iomem *pio, unsigned pin, u32 *div)
{
	*div = __raw_readl(pio + PIO_SCDR);

	return (__raw_readl(pio + PIO_IFSCSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_pulldown(void __iomem *pio, unsigned pin)
{
	return (__raw_readl(pio + PIO_PPDSR) >> pin) & 0x1;
}

static bool at91_mux_pio3_get_schmitt_trig(void __iomem *pio, unsigned pin)
{
	return (__raw_readl(pio + PIO_SCHMITT) >> pin) & 0x1;
}
#else
#define at91_mux_get_periph		NULL
#define at91_mux_pio3_get_periph	NULL
#define at91_mux_get_deglitch		NULL
#define at91_mux_pio3_get_debounce	NULL
#define at91_mux_pio3_get_pulldown	NULL
#define at91_mux_pio3_get_schmitt_trig	NULL
#endif

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

#ifdef CONFIG_CMD_AT91MUX
static void at91mux_printf_mode(unsigned bank, unsigned pin)
{
	struct at91_gpio_chip *at91_gpio = &gpio_chip[bank];
	void __iomem *pio = at91_gpio->regbase;
	enum at91_mux mode;
	u32 pdsr;

	unsigned mask = pin_to_mask(pin);

	mode = at91_gpio->ops->get_periph(pio, mask);

	if (mode == AT91_MUX_GPIO) {
		pdsr = __raw_readl(pio + PIO_PDSR);

		printf("[gpio] %s", pdsr & mask ? "set" : "clear");
	} else {
		printf("[periph %c]", mode + 'A' - 1);
	}
}

static void at91mux_dump_config(void)
{
	int bank, j;

	/* print heading */
	printf("Pin\t");
	for (bank = 0; bank < gpio_banks; bank++) {
		printf("PIO%c\t\t", 'A' + bank);
	};
	printf("\n\n");

	/* print pin status */
	for (j = 0; j < 32; j++) {
		printf("%i:\t", j);

		for (bank = 0; bank < gpio_banks; bank++) {
			at91mux_printf_mode(bank, j);

			printf("\t");
		}

		printf("\n");
	}
}

static void at91mux_print_en_disable(const char *str, bool is_on)
{
	printf("%s = ", str);

	if (is_on)
		printf("enable\n");
	else
		printf("disable\n");
}

static void at91mux_dump_pio_config(unsigned bank, unsigned pin)
{
	struct at91_gpio_chip *at91_gpio = &gpio_chip[bank];
	void __iomem *pio = at91_gpio->regbase;
	u32 div;

	printf("pio%c%d configuration\n\n", bank + 'A', pin);

	at91mux_printf_mode(bank, pin);
	printf("\n");

	at91mux_print_en_disable("multidrive",
		at91_mux_get_multidrive(pio, pin));

	at91mux_print_en_disable("pullup",
		at91_mux_get_pullup(pio, pin));

	if (at91_gpio->ops->get_deglitch)
		at91mux_print_en_disable("degitch",
			at91_gpio->ops->get_deglitch(pio, pin));

	if (at91_gpio->ops->get_debounce) {
		printf("debounce = ");
		if (at91_gpio->ops->get_debounce(pio, pin, &div))
			printf("enable at %d\n", div);
		else
			printf("disable\n");
	}

	if (at91_gpio->ops->get_pulldown)
		at91mux_print_en_disable("pulldown",
			at91_gpio->ops->get_pulldown(pio, pin));

	if (at91_gpio->ops->get_schmitt_trig)
		at91mux_print_en_disable("schmitt trigger",
			!at91_gpio->ops->get_schmitt_trig(pio, pin));
}

static int do_at91mux(int argc, char *argv[])
{
	int opt;
	unsigned bank = 0;
	unsigned pin = 0;

	if (argc < 2) {
		at91mux_dump_config();
		return 0;
	}

	while ((opt = getopt(argc, argv, "b:p:")) > 0) {
		switch (opt) {
		case 'b':
			bank = simple_strtoul(optarg, NULL, 10);
			break;
		case 'p':
			pin = simple_strtoul(optarg, NULL, 10);
			break;
		}
	}

	if (bank >= gpio_banks) {
		printf("bank %c >= supported %c banks\n", bank + 'A',
			gpio_banks + 'A');
		return 1;
	}

	if (pin >= 32) {
		printf("pin %d >= supported %d pins\n", pin, 32);
		return 1;
	}

	at91mux_dump_pio_config(bank, pin);

	return 0;
}

BAREBOX_CMD_HELP_START(at91mux)
BAREBOX_CMD_HELP_USAGE("at91mux [-p <pin> -b <bank>]\n")
BAREBOX_CMD_HELP_SHORT("dump current mux configuration if bank/pin specified dump pin details\n");
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(at91mux)
	.cmd		= do_at91mux,
	.usage		= "dump current mux configuration",
	BAREBOX_CMD_HELP(cmd_at91mux_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
#endif
/*--------------------------------------------------------------------------*/

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
	__raw_writel(mask, pio + PIO_OER);

	return 0;
}

static int at91_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct at91_gpio_chip *at91_gpio = to_at91_gpio_chip(chip);
	void __iomem *pio = at91_gpio->regbase;
	unsigned mask = 1 << offset;

	__raw_writel(mask, pio + PIO_ODR);

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
	.get = at91_gpio_get,
	.set = at91_gpio_set,
};

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

	gpio_banks = max(gpio_banks, dev->id + 1);
	at91_gpio->regbase = dev_request_mem_region(dev, 0);

	at91_gpio->chip.ops = &at91_gpio_ops;
	at91_gpio->chip.ngpio = MAX_NB_GPIO_PER_BANK;
	at91_gpio->chip.dev = dev;
	at91_gpio->chip.base = dev->id * MAX_NB_GPIO_PER_BANK;

	ret = gpiochip_add(&at91_gpio->chip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		return ret;
	}

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
