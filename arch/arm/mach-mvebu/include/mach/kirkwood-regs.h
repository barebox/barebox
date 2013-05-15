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

#define KIRKWOOD_INT_REGS_BASE IOMEM(0xd0000000)

#define KIRKWOOD_SDRAM_WIN_BASE (KIRKWOOD_INT_REGS_BASE + 0x1500)
#define  DDR_BASE_CS_OFF(n)     (0x0000 + ((n) << 3))
#define   DDR_BASE_CS_HIGH_MASK  0xf
#define   DDR_BASE_CS_LOW_MASK   0xff000000
#define  DDR_SIZE_CS_OFF(n)     (0x0004 + ((n) << 3))
#define   DDR_SIZE_ENABLED       (1 << 0)
#define   DDR_SIZE_CS_MASK       0x1c
#define   DDR_SIZE_CS_SHIFT      2
#define   DDR_SIZE_MASK          0xff000000
#define KIRKWOOD_SAR_BASE       (KIRKWOOD_INT_REGS_BASE + 0x10030)
#define  KIRKWOOD_TCLK_BIT      21
#define KIRKWOOD_UART_BASE      (KIRKWOOD_INT_REGS_BASE + 0x12000)
#define KIRKWOOD_CPUCTRL_BASE   (KIRKWOOD_INT_REGS_BASE + 0x20100)
#define KIRKWOOD_TIMER_BASE     (KIRKWOOD_INT_REGS_BASE + 0x20300)

#endif /* __MACH_MVEBU_KIRKWOOD_REGS_H */
