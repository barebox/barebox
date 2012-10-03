/*
 * Copyright (C) 2010 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <mach/ep93xx-regs.h>

#define EP93XX_GPIO_NUM_PORTS	8
#define EP93XX_GPIO_NUM_GPIOS	(EP93XX_GPIO_NUM_PORTS * 8)

struct gpio_port {
	uint32_t *dr;
	uint32_t *ddr;
};

struct gpio_port gpio_ports[EP93XX_GPIO_NUM_PORTS];

static int ep93xx_gpio_init(void)
{
	struct gpio_regs *gpio_regs = (struct gpio_regs *)GPIO_BASE;

	gpio_ports[0].dr = &gpio_regs->padr;
	gpio_ports[0].ddr = &gpio_regs->paddr;
	gpio_ports[1].dr = &gpio_regs->pbdr;
	gpio_ports[1].ddr = &gpio_regs->pbddr;
	gpio_ports[2].dr = &gpio_regs->pcdr;
	gpio_ports[2].ddr = &gpio_regs->pcddr;
	gpio_ports[3].dr = &gpio_regs->pddr;
	gpio_ports[3].ddr = &gpio_regs->pdddr;
	gpio_ports[4].dr = &gpio_regs->pedr;
	gpio_ports[4].ddr = &gpio_regs->peddr;
	gpio_ports[5].dr = &gpio_regs->pfdr;
	gpio_ports[5].ddr = &gpio_regs->pfddr;
	gpio_ports[6].dr = &gpio_regs->pgdr;
	gpio_ports[6].ddr = &gpio_regs->pgddr;
	gpio_ports[7].dr = &gpio_regs->phdr;
	gpio_ports[7].ddr = &gpio_regs->phddr;

	return 0;
}

postcore_initcall(ep93xx_gpio_init);

static struct gpio_port *gpio_get_port(unsigned gpio)
{
	if (gpio >= EP93XX_GPIO_NUM_GPIOS)
		return 0;

	return &gpio_ports[gpio / 8];
}

void gpio_set_value(unsigned gpio, int value)
{
	struct gpio_port *port = gpio_get_port(gpio);
	const int shift = gpio % 8;
	u32 val;

	if (!port)
		return;

	val = readl(port->dr);

	if (value)
		val |= 1 << shift;
	else
		val &= ~(1 << shift);

	writel(val, port->dr);
}

int gpio_direction_input(unsigned gpio)
{
	struct gpio_port *port = gpio_get_port(gpio);
	const int shift = gpio % 8;
	u32 val;

	if (!port)
		return -EINVAL;

	val = readl(port->ddr);
	val &= ~(1 << shift);
	writel(val, port->ddr);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	struct gpio_port *port = gpio_get_port(gpio);
	const int shift = gpio % 8;
	u32 val;

	if (!port)
		return -EINVAL;

	gpio_set_value(gpio, value);

	val = readl(port->ddr);
	val |= 1 << shift;
	writel(val, port->ddr);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	struct gpio_port *port = gpio_get_port(gpio);
	const int shift = gpio % 8;
	u32 val;

	if (!port)
		return -EINVAL;

	val = readl(port->dr);

	return val & (1 << shift) ? 1 : 0;
}

