/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2007 Andrew Victor */
/* SPDX-FileCopyrightText: 2007 Atmel Corporation */

/*
 * [origin: Linux kernel include/asm-arm/arch-at91/at91_pit.h]
 *
 * Periodic Interval Timer (PIT) - System peripherals regsters.
 * Based on AT91SAM9261 datasheet revision D.
 */

#ifndef AT91_PIT_H
#define AT91_PIT_H

#define AT91_PIT_MR		0x00			/* Mode Register */
#define		AT91_PIT_PITIEN		(1 << 25)		/* Timer Interrupt Enable */
#define		AT91_PIT_PITEN		(1 << 24)		/* Timer Enabled */
#define		AT91_PIT_PIV		(0xfffff)		/* Periodic Interval Value */

#define AT91_PIT_SR		0x04			/* Status Register */
#define		AT91_PIT_PITS		(1 << 0)		/* Timer Status */

#define AT91_PIT_PIVR		0x08			/* Periodic Interval Value Register */
#define AT91_PIT_PIIR		0x0c			/* Periodic Interval Image Register */
#define		AT91_PIT_PICNT		(0xfff << 20)		/* Interval Counter */
#define		AT91_PIT_CPIV		(0xfffff)		/* Inverval Value */

#endif
