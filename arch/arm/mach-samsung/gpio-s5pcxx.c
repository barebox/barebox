/*
 * Copyright (C) 2012 Alexey Galakhov
 * Copyright (C) 2012 Juergen Beisert, Pengutronix
 *
 * This codes bases partially on code from the Linux kernel:
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
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
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/s3c-iomap.h>

#define S3C_GPACON (S3C_GPIO_BASE)
#define S3C_GPADAT (S3C_GPIO_BASE + 0x04)
#define S3C_GPAPUD (S3C_GPIO_BASE + 0x08)

static inline unsigned group_offset(unsigned group)
{
    return group * 0x20;
}

void gpio_set_value(unsigned gpio, int value)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset = group_offset(group);
	uint32_t reg;

	reg = readl(S3C_GPADAT + offset);
	reg &= ~(1 << bit);
	reg |= (!!value) << bit;
	writel(reg, S3C_GPADAT + offset);
}

int gpio_get_value(unsigned gpio)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset = group_offset(group);
	uint32_t reg;

	/* value */
	reg = readl(S3C_GPADAT + offset);

	return !!(reg & (1 << bit));
}

int gpio_direction_input(unsigned gpio)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset = group_offset(group);
	uint32_t reg;

	bit <<= 2;
	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	writel(reg, S3C_GPACON + offset);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset = group_offset(group);
	uint32_t reg;

	gpio_set_value(gpio, value);

	bit <<= 2;
	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	reg |= 0x1 << bit;
	writel(reg, S3C_GPACON + offset);

	return 0;
}


/* 'gpio_mode' must be one of the 'GP?_*' macros */
void s3c_gpio_mode(unsigned gpio_mode)
{
	unsigned group = GET_GROUP(gpio_mode);
	unsigned bit = GET_BIT(gpio_mode);
	unsigned func = GET_FUNC(gpio_mode);
	unsigned offset = group_offset(group);
	unsigned reg;

	bit <<= 1;
	if (PUD_PRESENT(gpio_mode)) {
		reg = readl(S3C_GPAPUD + offset);
		reg &= ~(PUD_MASK << bit);
		reg |= GET_PUD(gpio_mode) << bit;
		writel(reg, S3C_GPAPUD + offset);
	}

	bit <<= 1;
	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	writel(reg | (func << bit), S3C_GPACON + offset);

	if (func == 1) { /* output? if yes, also set the initial value */
		reg = readl(S3C_GPADAT + offset) & ~(1 << (bit >> 2));
		reg |= GET_GPIOVAL(gpio_mode) << (bit >> 2);
		writel(reg, S3C_GPADAT + offset);
	}

}
