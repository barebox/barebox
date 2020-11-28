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

#ifndef _BCM2835_PLATFORM_H
#define _BCM2835_PLATFORM_H

/*
 *  SDRAM
 */
#define BCM2835_SDRAM_BASE	0x00000000

/*
 * Definitions and addresses for the ARM CONTROL logic
 * This file is manually generated.
 */

#define BCM2835_CACHELINE_SIZE	64

#define BCM2835_PL011_BASE 0x20201000
#define BCM2836_PL011_BASE 0x3f201000
#define BCM2835_MINIUART_BASE 0x20215040
#define BCM2836_MINIUART_BASE 0x3f215040

#endif

/* END */
