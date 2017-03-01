/*
 * Extract from arch/arm/mach-bcm2708/include/mach/platform.h
 *
 * Copyright (C) 2010 Broadcom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _WD_H
#define _WD_H

/*
 * Watchdog
 */
#define PM_RSTC		0x1c
#define PM_RSTS		0x20
#define PM_WDOG		0x24

#define PM_WDOG_RESET				0000000000
#define PM_PASSWORD				0x5a000000
#define PM_WDOG_TIME_SET			0x000fffff
#define PM_RSTC_WRCFG_CLR			0xffffffcf
#define PM_RSTC_WRCFG_SET			0x00000030
#define PM_RSTC_WRCFG_FULL_RESET		0x00000020
#define PM_RSTC_RESET				0x00000102

#define PM_RSTS_HADPOR_SET			0x00001000
#define PM_RSTS_HADSRH_SET			0x00000400
#define PM_RSTS_HADSRF_SET			0x00000200
#define PM_RSTS_HADSRQ_SET			0x00000100
#define PM_RSTS_HADWRH_SET			0x00000040
#define PM_RSTS_HADWRF_SET			0x00000020
#define PM_RSTS_HADWRQ_SET			0x00000010
#define PM_RSTS_HADDRH_SET			0x00000004
#define PM_RSTS_HADDRF_SET			0x00000002
#define PM_RSTS_HADDRQ_SET			0x00000001

#endif
