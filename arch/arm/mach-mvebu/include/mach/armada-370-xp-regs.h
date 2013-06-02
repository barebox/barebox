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

#ifndef __MACH_MVEBU_ARMADA_370_XP_REGS_H
#define __MACH_MVEBU_ARMADA_370_XP_REGS_H

#include <mach/common.h>

#define ARMADA_370_XP_INT_REGS_BASE	IOMEM(MVEBU_REMAP_INT_REG_BASE)
#define ARMADA_370_XP_UART_BASE		(ARMADA_370_XP_INT_REGS_BASE + 0x12000)
#define ARMADA_370_XP_UARTn_BASE(n)	\
	(ARMADA_370_XP_UART_BASE + ((n) * 0x100))

#define ARMADA_370_XP_SYSCTL_BASE	(ARMADA_370_XP_INT_REGS_BASE + 0x18200)
#define ARMADA_370_XP_SAR_BASE		(ARMADA_370_XP_INT_REGS_BASE + 0x18230)
#define  SAR_LOW			0x00
#define  SAR_TCLK_FREQ			BIT(20)
#define  SAR_HIGH			0x04

#define ARMADA_370_XP_SDRAM_BASE	(ARMADA_370_XP_INT_REGS_BASE + 0x20000)
#define  DDR_BASE_CS			0x180
#define  DDR_BASE_CSn(n)		(DDR_BASE_CS + ((n) * 0x8))
#define  DDR_BASE_CS_HIGH_MASK		0x0000000f
#define  DDR_BASE_CS_LOW_MASK		0xff000000
#define  DDR_SIZE_CS			0x184
#define  DDR_SIZE_CSn(n)		(DDR_SIZE_CS + ((n) * 0x8))
#define  DDR_SIZE_ENABLED		BIT(0)
#define  DDR_SIZE_CS_MASK		0x0000001c
#define  DDR_SIZE_CS_SHIFT		2
#define  DDR_SIZE_MASK			0xff000000

#define ARMADA_370_XP_TIMER_BASE	(ARMADA_370_XP_INT_REGS_BASE + 0x20300)

#endif /* __MACH_MVEBU_DOVE_REGS_H */
