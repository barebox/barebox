/*
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
#include <errno.h>
#include <io.h>
#include <mach/s3c-iomap.h>
#include <gpio.h>
#include <mach/s3c24xx-gpio.h>
#include <mach/iomux.h>

static const unsigned char group_offset[] =
{
	0x00,	/* GPA */
	0x10,	/* GPB */
	0x20,	/* GPC */
	0x30,	/* GPD */
	0x40,	/* GPE */
	0x50,	/* GPF */
	0x60,	/* GPG */
	0x70,	/* GPH */
#ifdef CONFIG_CPU_S3C2440
	0xd0,	/* GPJ */
#endif
};

void gpio_set_value(unsigned gpio, int value)
{
	unsigned group = gpio >> 5;
	unsigned bit = gpio % 32;
	unsigned offset;
	uint32_t reg;

	offset = group_offset[group];

	reg = readl(S3C_GPADAT + offset);
	reg &= ~(1 << bit);
	reg |= (!!value) << bit;
	writel(reg, S3C_GPADAT + offset);
}

int gpio_direction_input(unsigned gpio)
{
	unsigned group = gpio >> 5;
	unsigned bit = gpio % 32;
	unsigned offset;
	uint32_t reg;

	offset = group_offset[group];

	reg = readl(S3C_GPACON + offset);
	reg &= ~(0x3 << (bit << 1));
	writel(reg, S3C_GPACON + offset);

	return 0;
}


int gpio_direction_output(unsigned gpio, int value)
{
	unsigned group = gpio >> 5;
	unsigned bit = gpio % 32;
	unsigned offset;
	uint32_t reg;

	offset = group_offset[group];

	/* value */
	gpio_set_value(gpio,value);
	/* direction */
	if (group == 0) {	/* GPA is special */
		reg = readl(S3C_GPACON);
		reg &= ~(1 << bit);
		writel(reg, S3C_GPACON);
	} else {
		reg = readl(S3C_GPACON + offset);
		reg &= ~(0x3 << (bit << 1));
		reg |= 0x1 << (bit << 1);
		writel(reg, S3C_GPACON + offset);
	}

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	unsigned group = gpio >> 5;
	unsigned bit = gpio % 32;
	unsigned offset;
	uint32_t reg;

	if (group == 0)	/* GPA is special: no input mode available */
		return -ENODEV;

	offset = group_offset[group];

	/* value */
	reg = readl(S3C_GPADAT + offset);

	return !!(reg & (1 << bit));
}

void s3c_gpio_mode(unsigned gpio_mode)
{
	unsigned group, func, bit, offset, gpio;
	uint32_t reg;

	group = GET_GROUP(gpio_mode);
	func = GET_FUNC(gpio_mode);
	bit = GET_BIT(gpio_mode);
	gpio = GET_GPIO_NO(gpio_mode);

	if (group == 0) {
		/* GPA is special */
		switch (func) {
		case 0:		/* GPIO input */
			pr_debug("Cannot set GPA pin to GPIO input\n");
			break;
		case 1:		/* GPIO output */
			gpio_direction_output(bit, GET_GPIOVAL(gpio_mode));
			break;
		default:
			reg = readl(S3C_GPACON);
			reg |= 1 << bit;
			writel(reg, S3C_GPACON);
			break;
		}
		return;
	}

	offset = group_offset[group];

	if (PU_PRESENT(gpio_mode)) {
		reg = readl(S3C_GPACON + offset + 8);
		if (GET_PU(gpio_mode))
			reg |= (1 << bit);	/* set means _disabled_ */
		else
			reg &= ~(1 << bit);
		writel(reg, S3C_GPACON + offset + 8);
	}

	switch (func) {
	case 0: /* input */
		gpio_direction_input(gpio);
		break;
	case 1:	/* output */
		gpio_direction_output(gpio, GET_GPIOVAL(gpio_mode));
		break;
	case 2: /* function one */
	case 3: /* function two */
		reg = readl(S3C_GPACON + offset);
		reg &= ~(0x3 << (bit << 1));
		reg |= func << (bit << 1);
		writel(reg, S3C_GPACON + offset);
		break;
	}
}
