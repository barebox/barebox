/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * JZ4780 SoC definitions
 *
 * Copyright (c) 2013 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 */

#ifndef __MIPS_ASM_MACH_JZ4780_JZ4780_H__
#define __MIPS_ASM_MACH_JZ4780_JZ4780_H__

/* APB bus devices */
#define JZ4780_UART0_BASE	0xb0030000
#define JZ4780_UART1_BASE	0xb0031000
#define JZ4780_UART2_BASE	0xb0032000
#define JZ4780_UART3_BASE	0xb0033000
#define JZ4780_UART4_BASE	0xb0034000
#define JZ4780_UARTn_BASE(n)	(JZ4780_UART0_BASE + (n * 0x1000))

#endif /* __MIPS_ASM_MACH_JZ4780_JZ4780_H__ */
