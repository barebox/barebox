/*
 *  Generic PXA GPIO handling
 *
 *  Author:	Nicolas Pitre
 *  Created:	Jun 15, 2001
 *  Copyright:	MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <common.h>
#include <errno.h>

#include <mach/gpio.h>
#include <asm/io.h>

int pxa_last_gpio;

struct pxa_gpio_chip {
	void __iomem	*regbase;
};

static struct pxa_gpio_chip *pxa_gpio_chips;

#define for_each_gpio_chip(i, c) \
	for (i = 0, c = &pxa_gpio_chips[0]; i <= pxa_last_gpio; i += 32, c++)

static int __init pxa_init_gpio_chip(int gpio_end)
{
	int i, gpio, nbanks = gpio_to_bank(gpio_end) + 1;
	struct pxa_gpio_chip *chips;

	chips = kzalloc(nbanks * sizeof(struct pxa_gpio_chip), GFP_KERNEL);
	if (chips == NULL) {
		pr_err("%s: failed to allocate GPIO chips\n", __func__);
		return -ENOMEM;
	}

	for (i = 0, gpio = 0; i < nbanks; i++, gpio += 32)
		chips[i].regbase = (void __iomem *)GPIO_BANK(i);

	pxa_gpio_chips = chips;
	return 0;
}

int __init pxa_init_gpio(int start, int end)
{
	struct pxa_gpio_chip *c;
	int err,  gpio;

	pxa_last_gpio = end;

	/* Initialize GPIO chips */
	err = pxa_init_gpio_chip(end);
	if (err)
		return err;

	for_each_gpio_chip(gpio, c) {
		/* clear all GPIO edge detects */
		__raw_writel(0, c->regbase + GFER_OFFSET);
		__raw_writel(0, c->regbase + GRER_OFFSET);
		__raw_writel(~0, c->regbase + GEDR_OFFSET);
	}

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	return GPLR(gpio) & GPIO_bit(gpio);
}

void gpio_set_value(unsigned gpio, int value)
{
	if (value)
		GPSR(gpio) = GPIO_bit(gpio);
	else
		GPCR(gpio) = GPIO_bit(gpio);
}

int gpio_direction_input(unsigned gpio)
{
	if (__gpio_is_inverted(gpio))
		GPDR(gpio) |= GPIO_bit(gpio);
	else
		GPDR(gpio) &= ~GPIO_bit(gpio);
	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	gpio_set_value(gpio, value);
	if (__gpio_is_inverted(gpio))
		GPDR(gpio) &= ~GPIO_bit(gpio);
	else
		GPDR(gpio) |= GPIO_bit(gpio);
	return 0;
}
