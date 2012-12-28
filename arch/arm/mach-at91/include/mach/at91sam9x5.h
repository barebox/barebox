/*
 * Chip-specific header file for the AT91SAM9x5 family
 *
 *  Copyright (C) 2009-2010 Atmel Corporation.
 *
 * Common definitions.
 * Based on AT91SAM9x5 preliminary datasheet.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AT91SAM9X5_H
#define AT91SAM9X5_H

/*
 * Peripheral identifiers/interrupts.
 */
#define AT91_ID_FIQ		0	/* Advanced Interrupt Controller (FIQ) */
#define AT91_ID_SYS		1	/* System Controller Interrupt */
#define AT91SAM9X5_ID_PIOAB	2	/* Parallel I/O Controller A and B */
#define AT91SAM9X5_ID_PIOCD	3	/* Parallel I/O Controller C and D */
#define AT91SAM9X5_ID_SMD	4	/* SMD Soft Modem (SMD) */
#define AT91SAM9X5_ID_USART0	5	/* USART 0 */
#define AT91SAM9X5_ID_USART1	6	/* USART 1 */
#define AT91SAM9X5_ID_USART2	7	/* USART 2 */
#define AT91SAM9X5_ID_USART3	8	/* USART 3 */
#define AT91SAM9X5_ID_TWI0	9	/* Two-Wire Interface 0 */
#define AT91SAM9X5_ID_TWI1	10	/* Two-Wire Interface 1 */
#define AT91SAM9X5_ID_TWI2	11	/* Two-Wire Interface 2 */
#define AT91SAM9X5_ID_MCI0	12	/* High Speed Multimedia Card Interface 0 */
#define AT91SAM9X5_ID_SPI0	13	/* Serial Peripheral Interface 0 */
#define AT91SAM9X5_ID_SPI1	14	/* Serial Peripheral Interface 1 */
#define AT91SAM9X5_ID_UART0	15	/* UART 0 */
#define AT91SAM9X5_ID_UART1	16	/* UART 1 */
#define AT91SAM9X5_ID_TCB	17	/* Timer Counter 0, 1, 2, 3, 4 and 5 */
#define AT91SAM9X5_ID_PWM	18	/* Pulse Width Modulation Controller */
#define AT91SAM9X5_ID_ADC	19	/* ADC Controller */
#define AT91SAM9X5_ID_DMA0	20	/* DMA Controller 0 */
#define AT91SAM9X5_ID_DMA1	21	/* DMA Controller 1 */
#define AT91SAM9X5_ID_UHPHS	22	/* USB Host High Speed */
#define AT91SAM9X5_ID_UDPHS	23	/* USB Device High Speed */
#define AT91SAM9X5_ID_EMAC0	24	/* Ethernet MAC0 */
#define AT91SAM9X5_ID_LCDC	25	/* LCD Controller */
#define AT91SAM9X5_ID_ISI	25	/* Image Sensor Interface */
#define AT91SAM9X5_ID_MCI1	26	/* High Speed Multimedia Card Interface 1 */
#define AT91SAM9X5_ID_EMAC1	27	/* Ethernet MAC1 */
#define AT91SAM9X5_ID_SSC	28	/* Synchronous Serial Controller */
#define AT91SAM9X5_ID_CAN0	29	/* CAN Controller 0 */
#define AT91SAM9X5_ID_CAN1	30	/* CAN Controller 1 */
#define AT91SAM9X5_ID_IRQ0	31	/* Advanced Interrupt Controller */

/*
 * User Peripheral physical base addresses.
 */
#define AT91SAM9X5_BASE_SPI0		0xf0000000
#define AT91SAM9X5_BASE_SPI1		0xf0004000
#define AT91SAM9X5_BASE_MCI0		0xf0008000
#define AT91SAM9X5_BASE_MCI1		0xf000c000
#define AT91SAM9X5_BASE_SSC		0xf0010000
#define AT91SAM9X5_BASE_CAN0		0xf8000000
#define AT91SAM9X5_BASE_CAN1		0xf8004000
#define AT91SAM9X5_BASE_TCB0		0xf8008000
#define AT91SAM9X5_BASE_TC0		0xf8008000
#define AT91SAM9X5_BASE_TC1		0xf8008040
#define AT91SAM9X5_BASE_TC2		0xf8008080
#define AT91SAM9X5_BASE_TCB1		0xf800c000
#define AT91SAM9X5_BASE_TC3		0xf800c000
#define AT91SAM9X5_BASE_TC4		0xf800c040
#define AT91SAM9X5_BASE_TC5		0xf800c080
#define AT91SAM9X5_BASE_TWI0		0xf8010000
#define AT91SAM9X5_BASE_TWI1		0xf8014000
#define AT91SAM9X5_BASE_TWI2		0xf8018000
#define AT91SAM9X5_BASE_USART0		0xf801c000
#define AT91SAM9X5_BASE_USART1		0xf8020000
#define AT91SAM9X5_BASE_USART2		0xf8024000
#define AT91SAM9X5_BASE_USART3		0xf8028000
#define AT91SAM9X5_BASE_EMAC0		0xf802c000
#define AT91SAM9X5_BASE_EMAC1		0xf8030000
#define AT91SAM9X5_BASE_PWMC		0xf8034000
#define AT91SAM9X5_BASE_LCDC		0xf8038000
#define AT91SAM9X5_BASE_UDPHS		0xf803c000
#define AT91SAM9X5_BASE_UART0		0xf8040000
#define AT91SAM9X5_BASE_UART1		0xf8044000
#define AT91SAM9X5_BASE_ISI		0xf8048000
#define AT91SAM9X5_BASE_ADC		0xf804c000
#define AT91_BASE_SYS			0xffffc000

/*
 * System Peripherals
 */
#define AT91SAM9X5_BASE_MATRIX		0xffffde00
#define AT9SAM9X5_BASE1_PMECC		0xffffe000
#define AT91SAM9X5_BASE_PMERRLOC	0xffffe600
#define AT91SAM9X5_BASE_DDRSDRC0	0xffffe800
#define AT91SAM9X5_BASE_SMC		0xffffea00
#define AT91SAM9X5_BASE_DMA0		0xffffec00
#define AT91SAM9X5_BASE_DMA1		0xffffee00
#define AT91SAM9X5_BASE_AIC		0xfffff000
#define AT91SAM9X5_BASE_DBGU		0xfffff200
#define AT91SAM9X5_BASE_PIOA		0xfffff400
#define AT91SAM9X5_BASE_PIOB		0xfffff600
#define AT91SAM9X5_BASE_PIOC		0xfffff800
#define AT91SAM9X5_BASE_PIOD		0xfffffa00
#define AT91SAM9X5_BASE_PMC		0xfffffc00
#define AT91SAM9X5_BASE_RSTC		0xfffffe00
#define AT91SAM9X5_BASE_SHDWC		0xfffffe10
#define AT91SAM9X5_BASE_PIT		0xfffffe30
#define AT91SAM9X5_BASE_WDT		0xfffffe40
#define AT91SAM9X5_BASE_GPBR		0xfffffe60
#define AT91SAM9X5_BASE_RTC		0xfffffeb0

/*
 * System Peripherals (offset from AT91_BASE_SYS)
 */
#define AT91_MATRIX	(0xffffde00 - AT91_BASE_SYS)
#define AT91_PMECC	(0xffffe000 - AT91_BASE_SYS)
#define AT91_PMERRLOC	(0xffffe600 - AT91_BASE_SYS)
#define AT91_DDRSDRC0	(0xffffe800 - AT91_BASE_SYS)
#define AT91_DBGU	(0xfffff200 - AT91_BASE_SYS)
#define AT91_RSTC	(0xfffffe00 - AT91_BASE_SYS)
#define AT91_SHDWC	(0xfffffe10 - AT91_BASE_SYS)

#define AT91_BASE_WDT	AT91SAM9X5_BASE_WDT
#define AT91_BASE_SMC	AT91SAM9X5_BASE_SMC
#define AT91_BASE_PIOA	AT91SAM9X5_BASE_PIOA
#define AT91_BASE_PIOB	AT91SAM9X5_BASE_PIOB
#define AT91_BASE_PIOC	AT91SAM9X5_BASE_PIOC
#define AT91_BASE_PIOD	AT91SAM9X5_BASE_PIOD

#define AT91_USART0	AT91SAM9X5_BASE_US0
#define AT91_USART1	AT91SAM9X5_BASE_US1
#define AT91_USART2	AT91SAM9X5_BASE_US2
#define AT91_USART3	AT91SAM9X5_BASE_US3
#define AT91_NB_USART	5


/*
 * Internal Memory.
 */
#define AT91SAM9X5_SRAM_BASE	0x00300000	/* Internal SRAM base address */
#define AT91SAM9X5_SRAM_SIZE	SZ_32K		/* Internal SRAM size (32Kb) */

#define AT91SAM9X5_ROM_BASE	0x00400000	/* Internal ROM base address */
#define AT91SAM9X5_ROM_SIZE	SZ_64K		/* Internal ROM size (64Kb) */

#define AT91SAM9X5_SMD_BASE	0x00400000	/* SMD Controller */
#define AT91SAM9X5_UDPHS_FIFO	0x00500000	/* USB Device HS controller */
#define AT91SAM9X5_OHCI_BASE	0x00600000	/* USB Host controller (OHCI) */
#define AT91SAM9X5_EHCI_BASE	0x00700000	/* USB Host controller (EHCI) */

#define CONSISTENT_DMA_SIZE	SZ_4M

/*
 * DMA0 peripheral identifiers
 * for hardware handshaking interface
 */
#define AT_DMA_ID_MCI0		 0
#define AT_DMA_ID_SPI0_TX	 1
#define AT_DMA_ID_SPI0_RX	 2
#define AT_DMA_ID_USART0_TX	 3
#define AT_DMA_ID_USART0_RX	 4
#define AT_DMA_ID_USART1_TX	 5
#define AT_DMA_ID_USART1_RX	 6
#define AT_DMA_ID_TWI0_TX	 7
#define AT_DMA_ID_TWI0_RX	 8
#define AT_DMA_ID_TWI2_TX	 9
#define AT_DMA_ID_TWI2_RX	10
#define AT_DMA_ID_UART0_TX	11
#define AT_DMA_ID_UART0_RX	12
#define AT_DMA_ID_SSC_TX	13
#define AT_DMA_ID_SSC_RX	14

/*
 * DMA1 peripheral identifiers
 * for hardware handshaking interface
 */
#define AT_DMA_ID_MCI1		 0
#define AT_DMA_ID_SPI1_TX	 1
#define AT_DMA_ID_SPI1_RX	 2
#define AT_DMA_ID_SMD_TX	 3
#define AT_DMA_ID_SMD_RX	 4
#define AT_DMA_ID_TWI1_TX	 5
#define AT_DMA_ID_TWI1_RX	 6
#define AT_DMA_ID_ADC_RX	 7
#define AT_DMA_ID_DBGU_TX	 8
#define AT_DMA_ID_DBGU_RX	 9
#define AT_DMA_ID_UART1_TX	10
#define AT_DMA_ID_UART1_RX	11
#define AT_DMA_ID_USART2_TX	12
#define AT_DMA_ID_USART2_RX	13
#define AT_DMA_ID_USART3_TX	14
#define AT_DMA_ID_USART3_RX	15

#endif
