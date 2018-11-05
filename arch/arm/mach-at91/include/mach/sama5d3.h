/*
 * Chip-specific header file for the SAMA5D3 family
 *
 *  Copyright (C) 2009-2012 Atmel Corporation.
 *
 * Common definitions.
 * Based on SAMA5D3 datasheet.
 *
 * Licensed under GPLv2 or later.
 */

#ifndef SAMA5D3_H
#define SAMA5D3_H

/*
 * Peripheral identifiers/interrupts.
 */
#define SAMA5D3_ID_DBGU		 2	/* debug Unit (usually no special interrupt line) */
#define SAMA5D3_ID_PIT		 3	/* Periodic Interval Timer Interrupt */
#define SAMA5D3_ID_WDT		 4	/* Watchdog timer Interrupt */
#define SAMA5D3_ID_HSMC5	 5	/* Static Memory Controller */
#define SAMA5D3_ID_PIOA		 6	/* Parallel I/O Controller A */
#define SAMA5D3_ID_PIOB		 7	/* Parallel I/O Controller B */
#define SAMA5D3_ID_PIOC		 8	/* Parallel I/O Controller C */
#define SAMA5D3_ID_PIOD		 9	/* Parallel I/O Controller D */
#define SAMA5D3_ID_PIOE		10	/* Parallel I/O Controller E */
#define SAMA5D3_ID_SMD		11	/* SMD Soft Modem */
#define SAMA5D3_ID_USART0	12	/* USART0 */
#define SAMA5D3_ID_USART1	13	/* USART1 */
#define SAMA5D3_ID_USART2	14	/* USART2 */
#define SAMA5D3_ID_USART3	15	/* USART3 */
#define SAMA5D3_ID_UART0	16	/* UART0 */
#define SAMA5D3_ID_UART1	17	/* UART1 */
#define SAMA5D3_ID_TWI0		18	/* Two-Wire Interface 0 */
#define SAMA5D3_ID_TWI1		19	/* Two-Wire Interface 1 */
#define SAMA5D3_ID_TWI2		20	/* Two-Wire Interface 2 */
#define SAMA5D3_ID_HSMCI0	21	/* High Speed Multimedia Card Interface 0 */
#define SAMA5D3_ID_HSMCI1	22	/* High Speed Multimedia Card Interface 1 */
#define SAMA5D3_ID_HSMCI2	23	/* High Speed Multimedia Card Interface 2 */
#define SAMA5D3_ID_SPI0		24	/* Serial Peripheral Interface 0 */
#define SAMA5D3_ID_SPI1		25	/* Serial Peripheral Interface 1 */
#define SAMA5D3_ID_TC0		26	/* Timer Counter 0 (ch. 0, 1, 2) */
#define SAMA5D3_ID_TC1		27	/* Timer Counter 1 (ch. 3, 4, 5) */
#define SAMA5D3_ID_PWM		28	/* Pulse Width Modulation Controller */
#define SAMA5D3_ID_ADC		29	/* Touch Screen ADC Controller */
#define SAMA5D3_ID_DMA0		30	/* DMA Controller 0 */
#define SAMA5D3_ID_DMA1		31	/* DMA Controller 1 */
#define SAMA5D3_ID_UHPHS	32	/* USB Host High Speed */
#define SAMA5D3_ID_UDPHS	33	/* USB Device High Speed */
#define SAMA5D3_ID_GMAC		34	/* Gigabit Ethernet MAC */
#define SAMA5D3_ID_EMAC		35	/* Ethernet MAC */
#define SAMA5D3_ID_LCDC		36	/* LCD Controller */
#define SAMA5D3_ID_ISI		37	/* Image Sensor Interface */
#define SAMA5D3_ID_SSC0		38	/* Synchronous Serial Controller 0 */
#define SAMA5D3_ID_SSC1		39	/* Synchronous Serial Controller 1 */
#define SAMA5D3_ID_CAN0		40	/* CAN controller 0 */
#define SAMA5D3_ID_CAN1		41	/* CAN controller 1 */
#define SAMA5D3_ID_SHA		42	/* Secure Hash Algorithm */
#define SAMA5D3_ID_AES		43	/* Advanced Encryption Standard */
#define SAMA5D3_ID_TDES		44	/* Triple Data Encryption Standard */
#define SAMA5D3_ID_TRNG		45	/* True Random Number Generator */
#define SAMA5D3_ID_ARM		46	/* Performance Monitor Unit */
#define SAMA5D3_ID_AIC		47	/* Advanced Interrupt Controller */
#define SAMA5D3_ID_FUSE		48	/* Fuse Controller */
#define SAMA5D3_ID_MPDDRC	49	/* MPDDR controller */

/*
 * User Peripheral physical base addresses.
 */

#define SAMA5D3_BASE_HSMCI0	0xf0000000 /* (MMCI) Base Address */
#define SAMA5D3_BASE_SPI0	0xf0004000
#define SAMA5D3_BASE_TC0	0xf0010000 /* (TC0) Base Address */
#define SAMA5D3_BASE_TC1	0xf0010040 /* (TC1) Base Address */
#define	SAMA5D3_BASE_USART0	0xf001c000
#define	SAMA5D3_BASE_USART1	0xf0020000
#define SAMA5D3_BASE_GMAC	0xf0028000 /* (GMAC) Base Address */
#define SAMA5D3_BASE_LCDC	0xf0030000 /* (HLCDC5) Base Address */
#define SAMA5D3_BASE_HSMCI1	0xf8000000
#define SAMA5D3_BASE_HSMCI2	0xf8004000
#define SAMA5D3_BASE_SPI1	0xf8008000
#define SAMA5D3_BASE_EMAC	0xf802c000 /* (EMAC) Base Address */
#define SAMA5D3_BASE_UDPHS	0xf8030000

#define SAMA5D3_BASE_PIOA	0xfffff200
#define SAMA5D3_BASE_PIOB	0xfffff400
#define SAMA5D3_BASE_PIOC	0xfffff600
#define SAMA5D3_BASE_PIOD	0xfffff800
#define SAMA5D3_BASE_PIOE	0xfffffa00
#define SAMA5D3_BASE_MPDDRC	0xffffea00
#define	SAMA5D3_BASE_HSMC	0xffffc000
#define SAMA5D3_BASE_RSTC	0xfffffe00
#define SAMA5D3_BASE_PIT	0xfffffe30
#define SAMA5D3_BASE_WDT	0xfffffe40

#define SAMA5D3_BASE_PMECC	0xffffc070
#define SAMA5D3_BASE_PMERRLOC	0xffffc500

/*
 * Internal Memory.
 */
#define SAMA5D3_SRAM_BASE	0x00300000	/* Internal SRAM base address */
#define SAMA5D3_SRAM_SIZE	(128 * SZ_1K)	/* Internal SRAM size (128Kb) */

#define SAMA5D3_ROM_BASE	0x00100000
#define SAMA5D3_ROM_SIZE	SZ_1M

#define SAMA5D3_UDPHS_FIFO	0x00500000
#define SAMA5D3_OHCI_BASE	0x00600000	/* USB Host controller (OHCI) */
#define SAMA5D3_EHCI_BASE	0x00700000	/* USB Host controller (EHCI) */

#endif
