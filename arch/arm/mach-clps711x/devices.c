/*
 * Copyright (C) 2012 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <init.h>

#include <asm/io.h>

#include <mach/clps711x.h>

inline void _clps711x_setup_memcfg(int bank, u32 addr, u32 val)
{
	u32 tmp = readl(addr);

	tmp &= ~(0xff << (bank * 8));
	tmp |= val << (bank * 8);

	writel(tmp, addr);
}

void clps711x_setup_memcfg(int bank, u32 val)
{
	switch (bank) {
	case 0 ... 3:
		_clps711x_setup_memcfg(bank, MEMCFG1, val);
		break;
	case 4 ... 7:
		_clps711x_setup_memcfg(bank - 4, MEMCFG2, val);
		break;
	}
}

static struct resource uart0_resources[] = {
	{
		.start	= UBRLCR1,
		.end	= UBRLCR1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SYSCON1,
		.end	= SYSCON1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SYSFLG1,
		.end	= SYSFLG1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= UARTDR1,
		.end	= UARTDR1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource uart1_resources[] = {
	{
		.start	= UBRLCR2,
		.end	= UBRLCR2,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SYSCON2,
		.end	= SYSCON2,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SYSFLG2,
		.end	= SYSFLG2,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= UARTDR2,
		.end	= UARTDR2,
		.flags	= IORESOURCE_MEM,
	},
};

void clps711x_add_uart(unsigned int id)
{
	switch (id) {
	case 0:
		add_generic_device_res("clps711x_serial", 0, uart0_resources,
				       ARRAY_SIZE(uart0_resources), NULL);
		break;
	case 1:
		add_generic_device_res("clps711x_serial", 1, uart1_resources,
				       ARRAY_SIZE(uart1_resources), NULL);
		break;
	}
}
