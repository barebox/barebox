// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#ifndef _SUPERIO_H_
#define _SUPERIO_H_

#include <asm/io.h>
#include <linux/bitops.h>
#include <driver.h>
#include <linux/types.h>

#define SIO_REG_DEVID		0x20	/* Device ID (2 bytes) */
#define SIO_REG_DEVREV		0x22	/* Device revision */
#define SIO_REG_MANID		0x23	/* Vendor ID (2 bytes) */

static inline u8 superio_inb(u16 base, u8 reg)
{
	outb(reg, base);
	return inb(base + 1);
}

static inline u16 superio_inw(u16 base, u8 reg)
{
	u16 val;
	val  = superio_inb(base, reg) << 8;
	val |= superio_inb(base, reg + 1);
	return val;
}

static inline void superio_outb(u16 base, u8 reg, u8 val)
{
	outb(reg, base);
	outb(val, base + 1);
}

static inline void superio_set_bit(u16 base, u8 reg, unsigned bit)
{
	unsigned long val = superio_inb(base, reg);
	__set_bit(bit, &val);
	superio_outb(base, reg, val);
}

static inline void superio_clear_bit(u16 base, u8 reg, unsigned bit)
{
	unsigned long val = superio_inb(base, reg);
	__clear_bit(bit, &val);
	superio_outb(base, reg, val);
}

struct superio_chip {
	struct device_d *dev;
	u16 vid;
	u16 devid;
	u16 sioaddr;
	void (*enter)(u16 sioaddr);
	void (*exit)(u16 sioaddr);
};

void superio_chip_add(struct superio_chip *chip);
struct device_d *superio_func_add(struct superio_chip *chip, const char *name);

#endif
