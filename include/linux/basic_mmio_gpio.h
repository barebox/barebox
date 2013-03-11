/*
 * Basic memory-mapped GPIO controllers.
 *
 * Based on linux driver by:
 *  Copyright 2008 MontaVista Software, Inc.
 *  Copyright 2008,2010 Anton Vorontsov <cbouatmailru@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __BASIC_MMIO_GPIO_H
#define __BASIC_MMIO_GPIO_H

#include <common.h>
#include <gpio.h>
#include <io.h>

struct bgpio_pdata {
	int base;
	int ngpio;
};

struct bgpio_chip {
	struct gpio_chip gc;
	struct gpio_ops ops;

	unsigned int (*read_reg)(void __iomem *reg);
	void (*write_reg)(void __iomem *reg, unsigned int data);

	void __iomem *reg_dat;
	void __iomem *reg_set;
	void __iomem *reg_clr;
	void __iomem *reg_dir;

	/* Number of bits (GPIOs): <register width> * 8. */
	int bits;

	/*
	 * Some GPIO controllers work with the big-endian bits notation,
	 * e.g. in a 8-bits register, GPIO7 is the least significant bit.
	 */
	unsigned int (*pin2mask)(struct bgpio_chip *bgc, unsigned int pin);

	/* Shadowed data register to clear/set bits safely. */
	unsigned int data;

	/* Shadowed direction registers to clear/set direction safely. */
	unsigned int dir;
};

static inline struct bgpio_chip *to_bgpio_chip(struct gpio_chip *gc)
{
	return container_of(gc, struct bgpio_chip, gc);
}

int bgpio_init(struct bgpio_chip *bgc, struct device_d *dev,
	       unsigned int sz, void __iomem *dat, void __iomem *set,
	       void __iomem *clr, void __iomem *dirout, void __iomem *dirin,
	       unsigned long flags);
void bgpio_remove(struct bgpio_chip *bgc);

#define BGPIOF_BIG_ENDIAN		BIT(0)
#define BGPIOF_UNREADABLE_REG_SET	BIT(1) /* reg_set is unreadable */
#define BGPIOF_UNREADABLE_REG_DIR	BIT(2) /* reg_dir is unreadable */

#endif /* __BASIC_MMIO_GPIO_H */
