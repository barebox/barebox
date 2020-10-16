// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Du Huanpeng <u74147@gmail.com>
 */

#ifndef __ASM_MACH_LOONGSON1_PBL_LL_INIT_LOONGSON1_H
#define __ASM_MACH_LOONGSON1_PBL_LL_INIT_LOONGSON1_H

#include <asm/addrspace.h>
#include <asm/regdef.h>

.macro	__pbl_loongson1_ddr2_init
	.set	push
	.set	noreorder

	pbl_reg_writel 0x00000101, 0xAFFFFE00
	pbl_reg_writel 0x01000100, 0xAFFFFE04
	pbl_reg_writel 0x00000000, 0xAFFFFE10
	pbl_reg_writel 0x01000000, 0xAFFFFE14
	pbl_reg_writel 0x00000000, 0xAFFFFE20
	pbl_reg_writel 0x01000101, 0xAFFFFE24
	pbl_reg_writel 0x01000100, 0xAFFFFE30
	pbl_reg_writel 0x01010000, 0xAFFFFE34
	pbl_reg_writel 0x01010101, 0xAFFFFE40
	pbl_reg_writel 0x01000202, 0xAFFFFE44
	pbl_reg_writel 0x04030201, 0xAFFFFE50
	pbl_reg_writel 0x07000000, 0xAFFFFE54
	pbl_reg_writel 0x02020203, 0xAFFFFE60
	pbl_reg_writel 0x0a020203, 0xAFFFFE64
	pbl_reg_writel 0x00010506, 0xAFFFFE70
	pbl_reg_writel 0x00000400, 0xAFFFFE74
	pbl_reg_writel 0x08040201, 0xAFFFFE80
	pbl_reg_writel 0x08040201, 0xAFFFFE84
	pbl_reg_writel 0x00000000, 0xAFFFFE90
	pbl_reg_writel 0x00000306, 0xAFFFFE94
	pbl_reg_writel 0x3f0b020a, 0xAFFFFEA0
	pbl_reg_writel 0x0000003f, 0xAFFFFEA4
	pbl_reg_writel 0x00000000, 0xAFFFFEB0
	pbl_reg_writel 0x37570000, 0xAFFFFEB4
	pbl_reg_writel 0x08000000, 0xAFFFFEC0
	pbl_reg_writel 0x002a1503, 0xAFFFFEC4
	pbl_reg_writel 0x002a002a, 0xAFFFFED0
	pbl_reg_writel 0x002a002a, 0xAFFFFED4
	pbl_reg_writel 0x002a002a, 0xAFFFFEE0
	pbl_reg_writel 0x002a002a, 0xAFFFFEE4
	pbl_reg_writel 0x00000002, 0xAFFFFEF0
	pbl_reg_writel 0x00b40020, 0xAFFFFEF4
	pbl_reg_writel 0x00000087, 0xAFFFFF00
	pbl_reg_writel 0x000007ff, 0xAFFFFF04
	pbl_reg_writel 0x44240618, 0xAFFFFF10
	pbl_reg_writel 0x80808080, 0xAFFFFF14
	pbl_reg_writel 0x80808080, 0xAFFFFF20
	pbl_reg_writel 0x001c8080, 0xAFFFFF24
	pbl_reg_writel 0x00c8006b, 0xAFFFFF30
	pbl_reg_writel 0x36b00002, 0xAFFFFF34
	pbl_reg_writel 0x00c80017, 0xAFFFFF40
	pbl_reg_writel 0x00000000, 0xAFFFFF44
	pbl_reg_writel 0x00009c40, 0xAFFFFF50
	pbl_reg_writel 0x00000000, 0xAFFFFF54
	pbl_reg_writel 0x00000000, 0xAFFFFF60
	pbl_reg_writel 0x00000000, 0xAFFFFF64
	pbl_reg_writel 0x00000000, 0xAFFFFF70
	pbl_reg_writel 0x00000000, 0xAFFFFF74
	pbl_reg_writel 0x00000000, 0xAFFFFF80
	pbl_reg_writel 0x00000000, 0xAFFFFF84
	pbl_reg_writel 0x00000000, 0xAFFFFF90
	pbl_reg_writel 0x00000000, 0xAFFFFF94
	pbl_reg_writel 0x00000000, 0xAFFFFFA0
	pbl_reg_writel 0x00000000, 0xAFFFFFA4
	pbl_reg_writel 0x00000000, 0xAFFFFFB0
	pbl_reg_writel 0x00000000, 0xAFFFFFB4
	pbl_reg_writel 0x00000000, 0xAFFFFFC0
	pbl_reg_writel 0x00000000, 0xAFFFFFC4

	.set	pop
.endm

.macro	pbl_loongson1_ddr2_init
	.set	push
	.set	noreorder

	/* initial ddr2 controller */
	pbl_reg_writel 0xfc000000, 0xbfd010c8
	pbl_reg_writel 0x14000000, 0xbfd010f8

	__pbl_loongson1_ddr2_init
10:
	pbl_reg_writel  0x01010100, 0xaffffe34

9:
	li	t0, 0xaffffe00
	lw      t1, 0x10 (t0)
	andi    t1, t1, 1
	beqz    t1, 9b
	nop

	lh      t1, 0xf2 (t0)
	sltiu 	t1, t1, 5
	beqz 	t1, 1f
	nop

	lw      t1, 0xf4 (t0)
	addiu   t1, t1, 4
	sw      t1, 0xf4 (t0)
	b       10b
	nop
1:
	/* 16bit ddr and disable conf */
	pbl_reg_set 0x110000, 0xbfd00424
	.set	pop
.endm

#endif /* __ASM_MACH_LOONGSON1_PBL_LL_INIT_LOONGSON1_H */
