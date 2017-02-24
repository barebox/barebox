/*
 * Copyright (C) 2013
 *  Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *  Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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

#ifndef __MACH_COMMON_H__
#define __MACH_COMMON_H__

#include <asm/sections.h>
#include <asm/unaligned.h>

#define MVEBU_BOOTUP_INT_REG_BASE	0xd0000000
#define MVEBU_REMAP_INT_REG_BASE	0xf1000000

/* #including <asm/barebox-arm.h> yields a circle, so we need a forward decl */
uint32_t get_runtime_offset(void);

static inline void __iomem *mvebu_get_initial_int_reg_base(void)
{
#ifdef __PBL__
	u32 base = __get_unaligned_le32(_text - get_runtime_offset() + 0x30);
	return (void __force __iomem *)base;
#else
	return (void __force __iomem *)MVEBU_REMAP_INT_REG_BASE;
#endif
}

#endif
