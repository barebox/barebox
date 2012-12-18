/*
 * This file contains the address info for various AM33XX modules.
 *
 * Copyright (C) 2012 Teresa Gámez <t.gamez@phytec.de>,
 *		 Phytec Messtechnik GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_AM33XX_H
#define __ASM_ARCH_AM33XX_H

/** AM335x Internal Bus Base addresses */
#define AM33XX_L4_WKUP_BASE		0x44C00000
#define AM33XX_L4_PER_BASE		0x48000000
#define AM33XX_L4_FAST_BASE		0x4A000000

/* UART */
#define AM33XX_UART0_BASE		(AM33XX_L4_WKUP_BASE + 0x209000)
#define AM33XX_UART1_BASE		(AM33XX_L4_PER_BASE + 0x22000)
#define AM33XX_UART2_BASE		(AM33XX_L4_PER_BASE + 0x24000)

/* EMFI Registers */
#define AM33XX_EMFI0_BASE		0x4C000000

#define AM33XX_DRAM_ADDR_SPACE_START	0x80000000
#define AM33XX_DRAM_ADDR_SPACE_END	0xC0000000

/* GPMC */
#define AM33XX_GPMC_BASE		0x50000000

/* MMC */
#define AM33XX_MMCHS0_BASE		(AM33XX_L4_PER_BASE + 0x60000)

/* DTMTimer0 */
#define AM33XX_DMTIMER0_BASE		(AM33XX_L4_WKUP_BASE + 0x205000)

/* PRM */
#define AM33XX_PRM_BASE			(AM33XX_L4_WKUP_BASE + 0x200000)


#endif
