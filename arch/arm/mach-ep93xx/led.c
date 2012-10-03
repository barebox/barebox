/*
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
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
#include <io.h>
#include <mach/ep93xx-regs.h>

#define GREEN_LED_POS		0x01
#define RED_LED_POS		0x02

inline void switch_LED_on(uint32_t bit_pos)
{
	register struct gpio_regs *gpio = (struct gpio_regs *)GPIO_BASE;

	writel(readl(&gpio->pedr) | bit_pos, &gpio->pedr);
}

inline void switch_LED_off(uint32_t bit_pos)
{
	register struct gpio_regs *gpio = (struct gpio_regs *)GPIO_BASE;

	writel(readl(&gpio->pedr) & ~bit_pos, &gpio->pedr);
}

void red_LED_on(void)
{
	switch_LED_on(RED_LED_POS);
}

void red_LED_off(void)
{
	switch_LED_off(RED_LED_POS);
}

void green_LED_on(void)
{
	switch_LED_on(GREEN_LED_POS);
}

void green_LED_off(void)
{
	switch_LED_off(GREEN_LED_POS);
}
