#ifndef __ASM_MACH_LOONGSON1_PBL_MACROS_H
#define __ASM_MACH_LOONGSON1_PBL_MACROS_H

#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <mach/loongson1.h>

#define PLL_FREQ		0xBFE78030
#define PLL_DIV_PARAM		0xBFE78034

#define CONFIG_CPU_DIV		3
#define CONFIG_DDR_DIV		4
#define CONFIG_DC_DIV		4
#define CONFIG_PLL_FREQ		0x1C
#define CONFIG_PLL_DIV_PARAM	0x92392a00

.macro	pbl_loongson1_pll
	.set	push
	.set	noreorder

	pbl_reg_writel 0x92392a00, PLL_DIV_PARAM
	pbl_reg_writel 0x0000001c, PLL_FREQ
	pbl_sleep t8, 40

	.set	pop
.endm

.macro set_cpu_window id, base, mask, mmap
	.set	push
	.set	noreorder

	li	t8, 0xbfd00000
	sw	$0, 0x80 + \id * 8 (t8)
	li	t9, \base
	sw	t9, 0x00 + \id * 8 (t8)
	sw	$0, 0x04 + \id * 8 (t8)
	li	t9, \mask
	sw	t9, 0x40 + \id * 8 (t8)
	sw	$0, 0x44 + \id * 8 (t8)
	li	t9, \mmap
	sw	t9, 0x80 + \id * 8 (t8)
	sw	$0, 0x84 + \id * 8 (t8)

	.set	pop
.endm

.macro pbl_loongson1_remap
	.set	push

	set_cpu_window 0, 0x1c300000, 0xfff00000, 0x1c3000d2
	set_cpu_window 1, 0x1fe10000, 0xffffe000, 0x1fe100d3
	set_cpu_window 2, 0x1fe20000, 0xffffe000, 0x1fe200d3
	set_cpu_window 3, 0x1fe10000, 0xffff0000, 0x1fe100d0
	set_cpu_window 4, 0x1fe20000, 0xffff0000, 0x1fe200d0
	set_cpu_window 5, 0x1ff00000, 0xfff00000, 0x1ff000d0
	set_cpu_window 6, 0x1f000000, 0xff000000, 0x1f0000d3
	set_cpu_window 7, 0x00000000, 0x00000000, 0x000000f0
	li	t8, 0xbfd000e0
	lw	t9, 0x0 (t8)
	and	t9, t9, 0xffffff00
	ori	t9, t9, 0xd0
	sw	t9, 0x0 (t8)

	lw	t9, 0x8 (t8)
	and	t9, t9, 0xffffff00
	ori	t9, t9, 0xd0
	sw	t9, 0x8 (t8)

	.set	pop
.endm

#define GPIOCFG1	0xbfd010C4
.macro	pbl_loongson1_uart_enable
	.set	push

	pbl_reg_clr 0x00C00000, GPIOCFG1

	.set	pop
.endm

#endif /* __ASM_MACH_LOONGSON1_PBL_MACROS_H */
