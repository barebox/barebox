/*
 *
 * (c) 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
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

#define AT91_PIO3	1
#define MAX_GPIO_BANKS	5

static int gpio_banks;
static int cpu_has_pio3;

/*
 * Functionnality can change with newer chips
 */

struct at91_gpio_chip {
	void __iomem		*regbase;	/* PIO bank virtual address */
};

static struct at91_gpio_chip gpio_chip[MAX_GPIO_BANKS];

static inline void __iomem *pin_to_controller(unsigned pin)
{
	pin /= 32;
	if (likely(pin < gpio_banks))
		return gpio_chip[pin].regbase;

	return NULL;
}

static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << (pin % 32);
}

/*
 * mux the pin to the "GPIO" peripheral role.
 */
int at91_set_GPIO_periph(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;
	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	__raw_writel(mask, pio + PIO_PER);
	return 0;
}
EXPORT_SYMBOL(at91_set_GPIO_periph);

/*
 * mux the pin to the "A" internal peripheral role.
 */
int at91_set_A_periph(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	if (cpu_has_pio3) {
		__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask,
							pio + PIO_ABCDSR1);
		__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
							pio + PIO_ABCDSR2);
	} else {
		__raw_writel(mask, pio + PIO_ASR);
	}
	__raw_writel(mask, pio + PIO_PDR);
	return 0;
}
EXPORT_SYMBOL(at91_set_A_periph);

/*
 * mux the pin to the "B" internal peripheral role.
 */
int at91_set_B_periph(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	if (cpu_has_pio3) {
		__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask,
							pio + PIO_ABCDSR1);
		__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
							pio + PIO_ABCDSR2);
	} else {
		__raw_writel(mask, pio + PIO_BSR);
	}
	__raw_writel(mask, pio + PIO_PDR);
	return 0;
}
EXPORT_SYMBOL(at91_set_B_periph);

/*
 * mux the pin to the "C" internal peripheral role.
 */
int at91_set_C_periph(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !cpu_has_pio3)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
	__raw_writel(mask, pio + PIO_PDR);
	return 0;
}
EXPORT_SYMBOL(at91_set_C_periph);

/*
 * mux the pin to the "C" internal peripheral role.
 */
int at91_set_D_periph(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !cpu_has_pio3)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
	__raw_writel(mask, pio + PIO_PDR);
	return 0;
}
EXPORT_SYMBOL(at91_set_D_periph);

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral), and
 * configure it for an input.
 */
int at91_set_gpio_input(unsigned pin, int use_pullup)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	__raw_writel(mask, pio + PIO_ODR);
	__raw_writel(mask, pio + PIO_PER);
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_input);

/*
 * mux the pin to the gpio controller (instead of "A" or "B" peripheral),
 * and configure it for an output.
 */
int at91_set_gpio_output(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + PIO_PUDR);
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	__raw_writel(mask, pio + PIO_OER);
	__raw_writel(mask, pio + PIO_PER);
	return 0;
}
EXPORT_SYMBOL(at91_set_gpio_output);

/*
 * enable/disable the glitch filter; mostly used with IRQ handling.
 */
int at91_set_deglitch(unsigned pin, int is_on)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	if (cpu_has_pio3 && is_on)
		__raw_writel(mask, pio + PIO_IFSCDR);
	__raw_writel(mask, pio + (is_on ? PIO_IFER : PIO_IFDR));
	return 0;
}
EXPORT_SYMBOL(at91_set_deglitch);

/*
 * enable/disable the debounce filter;
 */
int at91_set_debounce(unsigned pin, int is_on, int div)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !cpu_has_pio3)
		return -EINVAL;

	if (is_on) {
		__raw_writel(mask, pio + PIO_IFSCER);
		__raw_writel(div & PIO_SCDR_DIV, pio + PIO_SCDR);
		__raw_writel(mask, pio + PIO_IFER);
	} else {
		__raw_writel(mask, pio + PIO_IFDR);
	}
	return 0;
}
EXPORT_SYMBOL(at91_set_debounce);

/*
 * enable/disable the multi-driver; This is only valid for output and
 * allows the output pin to run as an open collector output.
 */
int at91_set_multi_drive(unsigned pin, int is_on)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + (is_on ? PIO_MDER : PIO_MDDR));
	return 0;
}
EXPORT_SYMBOL(at91_set_multi_drive);

/*
 * enable/disable the pull-down.
 * If pull-up already enabled while calling the function, we disable it.
 */
int at91_set_pulldown(unsigned pin, int is_on)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !cpu_has_pio3)
		return -EINVAL;

	/* Disable pull-up anyway */
	__raw_writel(mask, pio + PIO_PUDR);
	__raw_writel(mask, pio + (is_on ? PIO_PPDER : PIO_PPDDR));
	return 0;
}
EXPORT_SYMBOL(at91_set_pulldown);

/*
 * disable Schmitt trigger
 */
int at91_disable_schmitt_trig(unsigned pin)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !cpu_has_pio3)
		return -EINVAL;

	__raw_writel(__raw_readl(pio + PIO_SCHMITT) | mask, pio + PIO_SCHMITT);
	return 0;
}
EXPORT_SYMBOL(at91_disable_schmitt_trig);

/*
 * assuming the pin is muxed as a gpio output, set its value.
 */
int at91_set_gpio_value(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
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
	void __iomem	*pio = pin_to_controller(pin);
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
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(__raw_readl(pio + PIO_PSR) & mask))
		return -EINVAL;
	__raw_writel(mask, pio + PIO_ODR);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_input);

int gpio_direction_output(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio || !(__raw_readl(pio + PIO_PSR) & mask))
		return -EINVAL;
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	__raw_writel(mask, pio + PIO_OER);
	return 0;
}
EXPORT_SYMBOL(gpio_direction_output);

/*--------------------------------------------------------------------------*/

static int at91_gpio_probe(struct device_d *dev)
{
	struct at91_gpio_chip *at91_gpio = &gpio_chip[dev->id];
	struct clk *clk;
	int ret;

	BUG_ON(dev->id > MAX_GPIO_BANKS);

	ret = dev_get_drvdata(dev, (unsigned long *)&cpu_has_pio3);
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

	return 0;
}

static struct platform_device_id at91_gpio_ids[] = {
	{
		.name = "at91rm9200-gpio",
		.driver_data = (unsigned long)0,
	}, {
		.name = "at91sam9x5-gpio",
		.driver_data = (unsigned long)AT91_PIO3,
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
