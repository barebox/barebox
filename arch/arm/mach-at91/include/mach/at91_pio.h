/*
 * [origin: Linux kernel include/asm-arm/arch-at91/at91_pio.h]
 *
 * Copyright (C) 2005 Ivan Kokshaysky
 * Copyright (C) SAN People
 * Copyright (C) 2015 Atmel,
 *               2015 Ludovic Desroches <ludovic.desroches@atmel.com>
 *
 * Parallel I/O Controller (PIO) - System peripherals registers.
 * Based on AT91RM9200 datasheet revision E.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AT91_PIO_H
#define AT91_PIO_H

#include <linux/bitops.h>

#define PIO_PER		0x00	/* Enable Register */
#define PIO_PDR		0x04	/* Disable Register */
#define PIO_PSR		0x08	/* Status Register */
#define PIO_OER		0x10	/* Output Enable Register */
#define PIO_ODR		0x14	/* Output Disable Register */
#define PIO_OSR		0x18	/* Output Status Register */
#define PIO_IFER	0x20	/* Glitch Input Filter Enable */
#define PIO_IFDR	0x24	/* Glitch Input Filter Disable */
#define PIO_IFSR	0x28	/* Glitch Input Filter Status */
#define PIO_SODR	0x30	/* Set Output Data Register */
#define PIO_CODR	0x34	/* Clear Output Data Register */
#define PIO_ODSR	0x38	/* Output Data Status Register */
#define PIO_PDSR	0x3c	/* Pin Data Status Register */
#define PIO_IER		0x40	/* Interrupt Enable Register */
#define PIO_IDR		0x44	/* Interrupt Disable Register */
#define PIO_IMR		0x48	/* Interrupt Mask Register */
#define PIO_ISR		0x4c	/* Interrupt Status Register */
#define PIO_MDER	0x50	/* Multi-driver Enable Register */
#define PIO_MDDR	0x54	/* Multi-driver Disable Register */
#define PIO_MDSR	0x58	/* Multi-driver Status Register */
#define PIO_PUDR	0x60	/* Pull-up Disable Register */
#define PIO_PUER	0x64	/* Pull-up Enable Register */
#define PIO_PUSR	0x68	/* Pull-up Status Register */
#define PIO_ASR		0x70	/* Peripheral A Select Register */
#define PIO_ABCDSR1	0x70	/* Peripheral ABCD Select Register 1 [some sam9 only] */
#define PIO_BSR		0x74	/* Peripheral B Select Register */
#define PIO_ABCDSR2	0x74	/* Peripheral ABCD Select Register 2 [some sam9 only] */
#define PIO_ABSR	0x78	/* AB Status Register */
#define PIO_IFSCDR	0x80	/* Input Filter Slow Clock Disable Register */
#define PIO_IFSCER	0x84	/* Input Filter Slow Clock Enable Register */
#define PIO_IFSCSR	0x88	/* Input Filter Slow Clock Status Register */
#define PIO_SCDR	0x8c	/* Slow Clock Divider Debouncing Register */
#define		PIO_SCDR_DIV	(0x3fff <<  0)		/* Slow Clock Divider Mask */
#define PIO_PPDDR	0x90	/* Pad Pull-down Disable Register */
#define PIO_PPDER	0x94	/* Pad Pull-down Enable Register */
#define PIO_PPDSR	0x98	/* Pad Pull-down Status Register */
#define PIO_OWER	0xa0	/* Output Write Enable Register */
#define PIO_OWDR	0xa4	/* Output Write Disable Register */
#define PIO_OWSR	0xa8	/* Output Write Status Register */
#define PIO_AIMER	0xb0	/* Additional Interrupt Modes Enable Register */
#define PIO_AIMDR	0xb4	/* Additional Interrupt Modes Disable Register */
#define PIO_AIMMR	0xb8	/* Additional Interrupt Modes Mask Register */
#define PIO_ESR		0xc0	/* Edge Select Register */
#define PIO_LSR		0xc4	/* Level Select Register */
#define PIO_ELSR	0xc8	/* Edge/Level Status Register */
#define PIO_FELLSR	0xd0	/* Falling Edge/Low Level Select Register */
#define PIO_REHLSR	0xd4	/* Rising Edge/ High Level Select Register */
#define PIO_FRLHSR	0xd8	/* Fall/Rise - Low/High Status Register */
#define PIO_SCHMITT	0x100	/* Schmitt Trigger Register */

#define ABCDSR_PERIPH_A	0x0
#define ABCDSR_PERIPH_B	0x1
#define ABCDSR_PERIPH_C	0x2
#define ABCDSR_PERIPH_D	0x3

#define PIO4_MSKR	0x0000	/* Mask Register */
#define PIO4_CFGR	0x0004	/* Configuration Register */
#define		PIO4_CFGR_FUNC_MASK		GENMASK(2, 0)
#define		PIO4_DIR_MASK			BIT(8)
#define		PIO4_PUEN_MASK			BIT(9)
#define		PIO4_PDEN_MASK			BIT(10)
#define		PIO4_IFEN_MASK			BIT(12)
#define		PIO4_IFSCEN_MASK		BIT(13)
#define		PIO4_OPD_MASK			BIT(14)
#define		PIO4_SCHMITT_MASK		BIT(15)
#define		PIO4_DRVSTR_MASK		GENMASK(17, 16)
#define		PIO4_DRVSTR_OFFSET		16
#define		PIO4_CFGR_EVTSEL_MASK		GENMASK(26, 24)
#define		PIO4_CFGR_EVTSEL_FALLING	(0 << 24)
#define		PIO4_CFGR_EVTSEL_RISING		(1 << 24)
#define		PIO4_CFGR_EVTSEL_BOTH		(2 << 24)
#define		PIO4_CFGR_EVTSEL_LOW		(3 << 24)
#define		PIO4_CFGR_EVTSEL_HIGH		(4 << 24)
#define	PIO4_PDSR	0x0008	/* Data Status Register */
#define	PIO4_LOCKSR	0x000C	/* Lock Status Register */
#define	PIO4_SODR	0x0010	/* Set Output Data Register */
#define	PIO4_CODR	0x0014	/* Clear Output Data Register */
#define	PIO4_ODSR	0x0018	/* Output Data Status Register */
#define	PIO4_IER	0x0020	/* Interrupt Enable Register */
#define	PIO4_IDR	0x0024	/* Interrupt Disable Register */
#define	PIO4_IMR	0x0028	/* Interrupt Mask Register */
#define	PIO4_ISR	0x002C	/* Interrupt Status Register */
#define	PIO4_IOFR	0x003C	/* I/O Freeze Configuration Register */

#endif
