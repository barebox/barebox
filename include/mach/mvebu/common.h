/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com> */
/* SPDX-FileCopyrightText: 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com> */

#ifndef __MACH_COMMON_H__
#define __MACH_COMMON_H__

#include <asm/sections.h>
#include <asm/unaligned.h>

#define MVEBU_BOOTUP_INT_REG_BASE	0xd0000000
#define MVEBU_REMAP_INT_REG_BASE	0xf1000000

/* #including <asm/barebox-arm.h> yields a circle, so we need a forward decl */
unsigned long get_runtime_offset(void);

static inline void __iomem *mvebu_get_initial_int_reg_base(void)
{
#ifdef __PBL__
	u32 base = __get_unaligned_le32(_text + get_runtime_offset() + 0x30);
	return (void __force __iomem *)base;
#else
	return (void __force __iomem *)MVEBU_REMAP_INT_REG_BASE;
#endif
}

#endif
