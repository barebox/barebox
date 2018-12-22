/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2011 Zhang, Keguang <keguang.zhang at gmail.com>
 *
 * Register mappings for Loongson 1
 */

#ifndef __ASM_MACH_LOONGSON1_LOONGSON1_H
#define __ASM_MACH_LOONGSON1_LOONGSON1_H

#include <asm/addrspace.h>

/* Loongson 1 Register Bases */
#define LS1X_UART0_BASE			0x1fe40000
#define LS1X_UART1_BASE			0x1fe44000
#define LS1X_UART2_BASE			0x1fe48000
#define LS1X_UART3_BASE			0x1fe4c000
#define LS1X_WDT_BASE			0x1fe5c060

/* Loongson 1 watchdog register definitions */
#define LS1X_WDT_REG(x) \
		((void __iomem *)KSEG1ADDR(LS1X_WDT_BASE + (x)))

#define LS1X_WDT_EN			LS1X_WDT_REG(0x0)
#define LS1X_WDT_SET			LS1X_WDT_REG(0x4)
#define LS1X_WDT_TIMER			LS1X_WDT_REG(0x8)

#endif /* __ASM_MACH_LOONGSON1_LOONGSON1_H */
