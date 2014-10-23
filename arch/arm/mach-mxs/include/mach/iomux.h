/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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

#ifndef __ASM_MACH_IOMUX_H
#define __ASM_MACH_IOMUX_H

#include <types.h>

/*
 * The muxable pins on i.MX23 are organized in 4 banks. On i.MX28 there are 7
 * banks. Each bank has up to 32 pins each. Furthermore for each pin some of the
 * following properties can be configured:
 *  - drive strength: 4 mA, 8 mA, 12 mA or 16 mA
 *  - pull up enabled or bit keeper enabled (a pin cannot have both)
 *  - voltage: 1.8 V, 2.5 V (i.MX23 only) or 3.3 V (i.MX28 only)
 *  - function: 0..3, with 3 being the GPIO functionality
 *
 * So a configuration for a given pin can be described in an unsigned integer of
 * length 32:
 *  - [ 4: 0] bank pin
 *  - [ 7: 5] bank
 *  - [    8] 1 iff pin has a switchable pull up
 *  - [    9] 1 iff pin has a switchable bit keeper
 *  - [   10] 1 iff pin has switchable drive strength
 *  - [   11] 1 iff pin has switchable voltage
 *  - [13:12] function
 *  - [   14] 1 for enabled pull up
 *  - [   15] 1 iff [14] is a valid pull up value
 *  - [   16] 1 for enabled bit keeper
 *  - [   17] 1 iff [16] is a valid bit keeper value
 *  - [19:18] value for drive strength i -> i * 4 mA
 *  - [   20] 1 iff [19:18] is valid
 *  - [   21] 0 for 1.8 V, 1 for 2.5 V resp. 3.3 V
 *  - [   22] 1 iff [21] is valid
 *  - [   23] 1 iff configure as GPIO out if function == 3 (i.e. GPIO)
 *  - [   24] initial value iff configured as GPIO out
 *  - [   25] error
 */

#define BANKPIN(p)	(((p) & 31) | ERROR((p) & ~31))
#define BANK(b)		((((b) & 7) << 5) | (ERROR((b) & ~7)))
#define PE		(1 << 8)
#define BK		(1 << 9)
#define SE		(1 << 10)
#define VE		(1 << 11)
#define FUNC(f)		((((f) & 3) << 12) | (ERROR((f) & ~3)))
#define PULLUP(p)	((((p) & 1) << 14) | PEVALID | ERROR((p) & ~1))
#define PEVALID		(1 << 15)
#define BITKEEPER(b)	((((b) & 1) << 16) | BKVALID | ERROR((b) & ~1))
#define BKVALID		(1 << 17)
#define STRENGTH(s)	((((s) & 3) << 18) | SEVALID | ERROR((s) & ~3))
#define S4MA		0
#define S8MA		1
#define S12MA		2
#define S16MA		3
#define SEVALID		(1 << 20)
#define VOLTAGE(v)	((((v) & 1) << 21) | VEVALID | ERROR((v) & ~1))
#define VE_1_8V		VOLTAGE(0)
#define VEVALID		(1 << 22)

#define GPIO_OUT	(1 << 23)
#define GPIO_IN		(0 << 23)
#define GPIO_VALUE(v)	((((v) & 1) << 24) | ERROR((v) & ~1))

#define ERROR(x)	(!!(x) << 25)

#define GET_GPIO_NO(m)	((m) & 0xff)
#define GET_FUNC(m)	(((m) >> 12) & 3)
#define PE_PRESENT(m)	((m) & PE)
#define GET_PULLUP(m)	(((m) >> 14) & 1)
#define BK_PRESENT(m)	((m) & BK)
#define GET_BITKEEPER(m)(((m) >> 16) & 1)
#define SE_PRESENT(m)	((m) & SE)
#define GET_STRENGTH(m)	(((m) >> 18) & 3)
#define VE_PRESENT(m)	((m) & VE)
#define GET_VOLTAGE(m)	(((m) >> 21) & 1)
#define GET_GPIODIR(m)	(!!((m) & GPIO_OUT))
#define GET_GPIOVAL(m)	(!!((m) & GPIO_VALUE(1)))
#define IS_GPIO		3

#if defined CONFIG_ARCH_IMX23
/*
 * The pin definition of i.MX23 are strange. Bank 0's pins 0 .. 15 are defined
 * using PORTF(0, 0) .. PORTF(0, 15). Its pins 16 .. 31 however use PORTF(1, 0)
 * .. PORTF(1, 15). So the PORTF macro is more ugly than necessary.
 */
# define PORTF(bank,bit)	(BANK((bank) / 2) | BANKPIN((((bank) & 1) << 4) | (bit)) | ERROR((bit) & ~15) | ERROR((bank) & ~7))
# define VE_2_5V		VOLTAGE(1)
# include <mach/iomux-imx23.h>
#endif

#if defined CONFIG_ARCH_IMX28
# define PORTF(bank,bit)	(BANK(bank) | BANKPIN(bit))
# define VE_3_3V	VOLTAGE(1)
# include <mach/iomux-imx28.h>
#endif

void imx_gpio_mode(uint32_t);

#endif /* __ASM_MACH_IOMUX_H */
