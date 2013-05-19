/*
 * Copyright
 * (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#ifndef __MACH_MVEBU_KIRKWOOD_REGS_H
#define __MACH_MVEBU_KIRKWOOD_REGS_H

#include <mach/common.h>

#define KIRKWOOD_INT_REGS_BASE	IOMEM(MVEBU_REMAP_INT_REG_BASE)

#define KIRKWOOD_SDRAM_BASE	(KIRKWOOD_INT_REGS_BASE + 0x00000)
#define DDR_BASE_CS		0x1500
#define DDR_BASE_CSn(n)		(DDR_BASE_CS + ((n) * 0x8))
#define  DDR_BASE_CS_HIGH_MASK	0x0000000f
#define  DDR_BASE_CS_LOW_MASK	0xff000000
#define DDR_SIZE_CS		0x1504
#define DDR_SIZE_CSn(n)		(DDR_SIZE_CS + ((n) * 0x8))
#define  DDR_SIZE_ENABLED	BIT(0)
#define  DDR_SIZE_CS_MASK	0x1c
#define  DDR_SIZE_CS_SHIFT	2
#define  DDR_SIZE_MASK		0xff000000

#define KIRKWOOD_SAR_BASE	(KIRKWOOD_INT_REGS_BASE + 0x10030)
#define  SAR_TCLK_FREQ		BIT(21)

#define KIRKWOOD_UART_BASE	(KIRKWOOD_INT_REGS_BASE + 0x12000)
#define KIRKWOOD_UARTn_BASE(n)	(KIRKWOOD_UART_BASE + ((n) * 0x100))

#define KIRKWOOD_BRIDGE_BASE	(KIRKWOOD_INT_REGS_BASE + 0x20000)
#define  BRIDGE_RSTOUT_MASK	0x108
#define  SOFT_RESET_OUT_EN	BIT(2)
#define  BRIDGE_SYS_SOFT_RESET	0x10c
#define  SOFT_RESET_EN		BIT(0)

#define KIRKWOOD_TIMER_BASE	(KIRKWOOD_INT_REGS_BASE + 0x20300)

#endif /* __MACH_MVEBU_KIRKWOOD_REGS_H */
