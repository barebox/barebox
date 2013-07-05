/*
 * Copyright
 * (C) 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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

#ifndef __MACH_MVEBU_DOVE_REGS_H
#define __MACH_MVEBU_DOVE_REGS_H

#include <mach/common.h>

/*
 * Even after MVEBU SoC internal register base remap. Dove MC
 * registers are still at 0xd0800000. We remap it right after
 * internal registers to 0xf1800000.
*/
#define DOVE_BOOTUP_MC_REGS	0xd0800000
#define DOVE_REMAP_MC_REGS	0xf1800000

#define DOVE_INT_REGS_BASE	IOMEM(MVEBU_REMAP_INT_REG_BASE)
#define DOVE_MC_REGS_BASE	IOMEM(DOVE_REMAP_MC_REGS)

#define DOVE_UART_BASE		(DOVE_INT_REGS_BASE + 0x12000)
#define DOVE_UARTn_BASE(n)	(DOVE_UART_BASE + ((n) * 0x100))

#define DOVE_SPI0_BASE		(DOVE_INT_REGS_BASE + 0x10600)
#define DOVE_SPI1_BASE		(DOVE_INT_REGS_BASE + 0x14600)

#define DOVE_BRIDGE_BASE	(DOVE_INT_REGS_BASE + 0x20000)
#define  INT_REGS_BASE_MAP	0x080
#define  BRIDGE_RSTOUT_MASK	0x108
#define  SOFT_RESET_OUT_EN	BIT(2)
#define  BRIDGE_SYS_SOFT_RESET	0x10c
#define  SOFT_RESET_EN		BIT(0)
#define DOVE_TIMER_BASE		(DOVE_INT_REGS_BASE + 0x20300)

#define DOVE_SAR_BASE		(DOVE_INT_REGS_BASE + 0xd0214)
#define  SAR0			0x000
#define  TCLK_FREQ_SHIFT	23
#define  TCLK_FREQ_MASK		(0x3 << TCLK_FREQ_SHIFT)
#define  SAR1			0x004

#define DOVE_AXI_CTRL		(DOVE_INT_REGS_BASE + 0xd0224)
#define DOVE_CPU_CTRL		(DOVE_INT_REGS_BASE + 0xd025c)

#define DOVE_SDRAM_BASE		(DOVE_MC_REGS_BASE)
#define  SDRAM_REGS_BASE_DECODE	0x010
#define  SDRAM_MAPn(n)		(0x100 + ((n) * 0x10))
#define  SDRAM_START_MASK	(0x1ff << 23)
#define  SDRAM_LENGTH_SHIFT	16
#define  SDRAM_LENGTH_MASK	(0x00f << SDRAM_LENGTH_SHIFT)
#define  SDRAM_ADDRESS_MASK	(0x1ff << 7)
#define  SDRAM_MAP_VALID	BIT(0)

#endif /* __MACH_MVEBU_DOVE_REGS_H */
