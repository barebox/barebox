/*
 * [origin: Linux kernel include/asm-arm/arch-at91/hardware.h]
 *
 *  Copyright (C) 2003 SAN People
 *  Copyright (C) 2003 ATMEL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#if defined(CONFIG_ARCH_AT91RM9200)
#include <mach/at91rm9200.h>
#elif defined(CONFIG_ARCH_AT91SAM9260) || defined(CONFIG_ARCH_AT91SAM9G20)
#include <mach/at91sam9260.h>
#elif defined(CONFIG_ARCH_AT91SAM9261) || defined(CONFIG_ARCH_AT91SAM9G10)
#include <mach/at91sam9261.h>
#elif defined(CONFIG_ARCH_AT91SAM9263)
#include <mach/at91sam9263.h>
#elif defined(CONFIG_ARCH_AT91SAM9RL)
#include <mach/at91sam9rl.h>
#elif defined(CONFIG_ARCH_AT91SAM9G45)
#include <mach/at91sam9g45.h>
#elif defined(CONFIG_ARCH_AT91CAP9)
#include <mach/at91cap9.h>
#elif defined(CONFIG_ARCH_AT91X40)
#include <mach/at91x40.h>
#else
#error "Unsupported AT91 processor"
#endif

/* External Memory Map */
#define AT91_CHIPSELECT_0	0x10000000
#define AT91_CHIPSELECT_1	0x20000000
#define AT91_CHIPSELECT_2	0x30000000
#define AT91_CHIPSELECT_3	0x40000000
#define AT91_CHIPSELECT_4	0x50000000
#define AT91_CHIPSELECT_5	0x60000000
#define AT91_CHIPSELECT_6	0x70000000
#define AT91_CHIPSELECT_7	0x80000000

/* SDRAM */
#ifdef CONFIG_DRAM_BASE
#define AT91_SDRAM_BASE		CONFIG_DRAM_BASE
#else
#define AT91_SDRAM_BASE		AT91_CHIPSELECT_1
#endif

/* Clocks */
#define AT91_SLOW_CLOCK		32768		/* slow clock */

#endif
