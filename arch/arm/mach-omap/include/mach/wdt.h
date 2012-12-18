/**
 * @file
 * @brief This file contains the Watchdog timer specific register definitions
 *
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
 */

#ifndef __ASM_ARCH_OMAP_WDT_H
#define __ASM_ARCH_OMAP_WDT_H

/** Watchdog Register defines */
#define OMAP3_WDT_REG(REGNAME)	(OMAP3_MPU_WDTIMER_BASE + OMAP_WDT_##REGNAME)
#define AM33XX_WDT_REG(REGNAME)	(AM33XX_WDT_BASE + OMAP_WDT_##REGNAME)

#define OMAP_WDT_WIDR		(0x000)
#define OMAP_WDT_SYSCONFIG	(0x010)
#define OMAP_WDT_WD_SYSSTATUS	(0x014)
#define OMAP_WDT_WISR		(0x018)
#define OMAP_WDT_WIER		(0x01C)
#define OMAP_WDT_WCLR		(0x024)
#define OMAP_WDT_WCRR		(0x028)
#define OMAP_WDT_WLDR		(0x02C)
#define OMAP_WDT_WTGR		(0x030)
#define OMAP_WDT_WWPS		(0x034)
#define OMAP_WDT_WSPR		(0x048)

/* Unlock Code for Watchdog timer to disable the same */
#define WDT_DISABLE_CODE1	0xAAAA
#define WDT_DISABLE_CODE2	0x5555

#endif /* __ASM_ARCH_OMAP_WDT_H */
