/*
 * Copyright (C) 2012 Juergen Beisert
 *
 * This code bases partially on code from the Linux kernel:
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

static const unsigned short group_offset[] = {
	0x000,	/* GPA */ /* 8 pins, 4 bit each */
	0x020,	/* GPB */ /* 7 pins, 4 bit each */
	0x040,	/* GPC */ /* 8 pins, 4 bit each */
	0x060,	/* GPD */ /* 5 pins, 4 bit each */
	0x080,	/* GPE */ /* 5 pins, 4 bit each */
	0x0a0,	/* GPF */ /* 16 pins, 2 bit each */
	0x0c0,	/* GPG */ /* 7 pins, 4 bit each */
	0x0e0,	/* GPH */ /* two registers, 8 + 2 pins, 4 bit each */
	0x100,	/* GPI */ /* 16 pins, 2 bit each */
	0x120,	/* GPJ */ /* 12 pins, 2 bit each */
	0x800,	/* GPK */ /* two registers, 8 + 8 pins, 4 bit each */
	0x810,	/* GPL */ /* two registers, 8 + 8 pins, 4 bit each */
	0x820,	/* GPM */ /* 6 pins, 4 bit each */
	0x830,	/* GPN */ /* 16 pins, 2 bit each */
	0x140,	/* GPO */ /* 16 pins, 2 bit each */
	0x160,	/* GPP */ /* 15 pins, 2 bit each */
	0x180,	/* GPQ */ /* 9 pins, 2 bit each */
};

void gpio_set_value(unsigned gpio, int value)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset;
	uint32_t reg;

	offset = group_offset[group];

	switch (group) {
	case 7:		/* GPH */
	case 10:	/* GPK */
	case 11:	/* GPL */
		offset += 4;
		break;
	}

	reg = readl(S3C_GPADAT + offset);
	reg &= ~(1 << bit);
	reg |= (!!value) << bit;
	writel(reg, S3C_GPADAT + offset);
}

int gpio_get_value(unsigned gpio)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset;
	uint32_t reg;

	offset = group_offset[group];

	switch (group) {
	case 7:		/* GPH */
	case 10:	/* GPK */
	case 11:	/* GPL */
		offset += 4;
		break;
	}

	/* value */
	reg = readl(S3C_GPADAT + offset);

	return !!(reg & (1 << bit));
}

static void gpio_direction_input_4b(unsigned offset, unsigned bit)
{
	uint32_t reg;

	if (bit > 31) {
		offset += 4;
		bit -= 32;
	}

	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	writel(reg, S3C_GPACON + offset); /* b0000 means 'GPIO input' */
}

static void gpio_direction_input_2b(unsigned offset, unsigned bit)
{
	uint32_t reg;

	reg = readl(S3C_GPACON + offset) & ~(0x3 << bit);
	writel(reg, S3C_GPACON + offset); /* b00 means 'GPIO input' */
}

int gpio_direction_input(unsigned gpio)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset;

	offset = group_offset[group];

	switch (group) {
	case 5:		/* GPF */
	case 8:		/* GPI */
	case 9:		/* GPJ */
	case 13:	/* GPN */
	case 14:	/* GPO */
	case 15:	/* GPP */
	case 16:	/* GPQ */
		gpio_direction_input_2b(offset, bit << 1);
		break;
	default:
		gpio_direction_input_4b(offset, bit << 2);
	}

	return 0;
}

static void gpio_direction_output_4b(unsigned offset, unsigned bit)
{
	uint32_t reg;

	if (bit > 31) {
		offset += 4;
		bit -= 32;
	}

	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	reg |= 0x1 << bit;
	writel(reg, S3C_GPACON + offset); /* b0001 means 'GPIO output' */
}

static void gpio_direction_output_2b(unsigned offset, unsigned bit)
{
	uint32_t reg;

	/* direction */
	reg = readl(S3C_GPACON + offset) & ~(0x3 << bit);
	reg |= 0x1 << bit;
	writel(reg, S3C_GPACON + offset);
}

int gpio_direction_output(unsigned gpio, int value)
{
	unsigned group = GET_GROUP(gpio);
	unsigned bit = GET_BIT(gpio);
	unsigned offset;

	gpio_set_value(gpio, value);

	offset = group_offset[group];
	switch (group) {
	case 5:		/* GPF */
	case 8:		/* GPI */
	case 9:		/* GPJ */
	case 13:	/* GPN */
	case 14:	/* GPO */
	case 15:	/* GPP */
	case 16:	/* GPQ */
		gpio_direction_output_2b(offset, bit << 1);
		break;
	default:
		gpio_direction_output_4b(offset, bit << 2);
	}

	return 0;
}

/* one register, 2 bits per function -> GPF, GPI, GPJ, GPN, GPO, GPP, GPQ */
static void s3c_d2pins(unsigned offset, unsigned pin_mode)
{
	unsigned bit = GET_BIT(pin_mode);
	unsigned func = GET_FUNC(pin_mode);
	unsigned reg;

	if (PUD_PRESENT(pin_mode)) {
		reg = readl(S3C_GPAPUD + offset);
		reg &= ~(PUD_MASK << bit);
		reg |= GET_PUD(pin_mode) << bit;
		writel(reg, S3C_GPAPUD + offset);
	}

	/* in the case of pin's function is GPIO it also sets up the direction */
	reg = readl(S3C_GPACON + offset) & ~(0x3 << bit);
	writel(reg | (func << bit), S3C_GPACON + offset);

	if (func == 1) { /* output? if yes, also set the initial value */
		reg = readl(S3C_GPADAT + offset) & ~(1 << (bit >> 1));
		reg |= GET_GPIOVAL(pin_mode) << (bit >> 1);
		writel(reg, S3C_GPADAT + offset);
	}
}

/* one register, 4 bits per function -> GPA, GPB, GPC, GPD, GPE, GPG, GPM */
static void s3c_d4pins(unsigned offset, unsigned pin_mode)
{
	unsigned bit = GET_BIT(pin_mode);
	unsigned func = GET_FUNC(pin_mode);
	unsigned reg;

	if (PUD_PRESENT(pin_mode)) {
		reg = readl(S3C_GPAPUD + offset);
		reg &= ~(PUD_MASK << (bit >> 1));
		reg |= GET_PUD(pin_mode) << (bit >> 1);
		writel(reg, S3C_GPAPUD + offset);
	}

	/* in the case of pin's function is GPIO it also sets up the direction */
	reg = readl(S3C_GPACON + offset) & ~(0xf << bit);
	writel(reg | (func << bit), S3C_GPACON + offset);

	if (func == 1) { /* output? if yes, also set the initial value */
		reg = readl(S3C_GPADAT + offset) & ~(1 << (bit >> 2));
		reg |= GET_GPIOVAL(pin_mode) << (bit >> 2);
		writel(reg, S3C_GPADAT + offset);
	}
}

/* two registers, 4 bits per pin -> GPH, GPK, GPL */
static void s3c_d42pins(unsigned offset, unsigned pin_mode)
{
	unsigned bit = GET_BIT(pin_mode);
	unsigned func = GET_FUNC(pin_mode);
	uint32_t reg;
	unsigned reg_offs = 0;

	if (PUD_PRESENT(pin_mode)) {
		reg = readl(S3C_GPAPUD + 4 + offset);
		reg &= ~(PUD_MASK << (bit >> 1));
		reg |= GET_PUD(pin_mode) << (bit >> 1);
		writel(reg, S3C_GPACON + 4 + offset);
	}

	if (bit > 31) {
		reg_offs = 4;
		bit -= 32;
	}

	/* in the case of pin's function is GPIO it also sets up the direction */
	reg = readl(S3C_GPACON + offset + reg_offs) & ~(0xf << bit);
	writel(reg | (func << bit), S3C_GPACON + offset + reg_offs);

	if (func == 1) { /* output? if yes, also set the initial value */
		reg = readl(S3C_GPADAT + 4 + offset) & ~(1 << (bit >> 2));
		reg |= GET_GPIOVAL(pin_mode) << (bit >> 2);
		writel(reg, S3C_GPADAT + 4 + offset);
	}
}

/* 'gpio_mode' must be one of the 'GP?_*' macros */
void s3c_gpio_mode(unsigned gpio_mode)
{
	unsigned group, offset;

	group = GET_GROUP(gpio_mode);
	offset = group_offset[group];

	switch (group) {
	case 5:		/* GPF */
	case 8:		/* GPI */
	case 9:		/* GPJ */
	case 13:	/* GPN */
	case 14:	/* GPO */
	case 15:	/* GPP */
	case 16:	/* GPQ */
		s3c_d2pins(offset, gpio_mode);
		break;
	case 7:		/* GPH */
	case 10:	/* GPK */
	case 11:	/* GPL */
		s3c_d42pins(offset, gpio_mode);
		break;
	default:
		s3c_d4pins(offset, gpio_mode);
	}
}
