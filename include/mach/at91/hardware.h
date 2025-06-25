/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2003 SAN People */
/* SPDX-FileCopyrightText: 2003 ATMEL */

/* [origin: Linux kernel include/asm-arm/arch-at91/hardware.h] */

#ifndef __MACH_AT91_HARDWARE_H
#define __MACH_AT91_HARDWARE_H

/* DBGU base */
/* rm9200, 9260/9g20, 9261/9g10, 9rl */
#define AT91_BASE_DBGU0	0xfffff200
/* 9263, 9g45 */
#define AT91_BASE_DBGU1	0xffffee00

#include <mach/at91/at91rm9200.h>
#include <mach/at91/at91sam9260.h>
#include <mach/at91/at91sam9261.h>
#include <mach/at91/at91sam9263.h>
#include <mach/at91/at91sam9g45.h>
#include <mach/at91/at91sam9n12.h>
#include <mach/at91/at91sam9x5.h>
#include <mach/at91/sama5d2.h>
#include <mach/at91/sama5d3.h>
#include <mach/at91/sama5d4.h>

/* External Memory Map */
#define AT91_CHIPSELECT_0	0x10000000
#define AT91_CHIPSELECT_1	0x20000000
#define AT91_CHIPSELECT_2	0x30000000
#define AT91_CHIPSELECT_3	0x40000000
#define AT91_CHIPSELECT_4	0x50000000
#define AT91_CHIPSELECT_5	0x60000000
#define AT91_CHIPSELECT_6	0x70000000
#define AT91_CHIPSELECT_7	0x80000000

#define SAMA5_CHIPSELECT_0	0x10000000
#define SAMA5_DDRCS		0x20000000
#define SAMA5_CHIPSELECT_1	0x40000000
#define SAMA5_CHIPSELECT_2	0x50000000
#define SAMA5_CHIPSELECT_3	0x60000000

/* Clocks */
#define AT91_SLOW_CLOCK		32768		/* slow clock */

#endif /* __MACH_AT91_HARDWARE_H */
