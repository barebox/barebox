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
