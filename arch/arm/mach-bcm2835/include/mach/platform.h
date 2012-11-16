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

#define BCM2835_PERI_BASE	0x20000000
#define BCM2835_ST_BASE		(BCM2835_PERI_BASE + 0x3000)	/* System Timer */
#define BCM2835_DMA_BASE	(BCM2835_PERI_BASE + 0x7000)	/* DMA controller */
#define BCM2835_ARM_BASE	(BCM2835_PERI_BASE + 0xB000)	/* BCM2708 ARM control block */
#define BCM2835_PM_BASE		(BCM2835_PERI_BASE + 0x100000)	/* Power Management, Reset controller and Watchdog registers */
#define BCM2835_GPIO_BASE	(BCM2835_PERI_BASE + 0x200000)	/* GPIO */
#define BCM2835_UART0_BASE	(BCM2835_PERI_BASE + 0x201000)	/* Uart 0 */
#define BCM2835_MMCI0_BASE	(BCM2835_PERI_BASE + 0x202000)	/* MMC interface */
#define BCM2835_SPI0_BASE	(BCM2835_PERI_BASE + 0x204000)	/* SPI0 */
#define BCM2835_BSC0_BASE	(BCM2835_PERI_BASE + 0x205000)	/* BSC0 I2C/TWI */
#define BCM2835_UART1_BASE	(BCM2835_PERI_BASE + 0x215000)	/* Uart 1 */
#define BCM2835_EMMC_BASE	(BCM2835_PERI_BASE + 0x300000)	/* eMMC interface */
#define BCM2835_SMI_BASE	(BCM2835_PERI_BASE + 0x600000)	/* SMI */
#define BCM2835_BSC1_BASE	(BCM2835_PERI_BASE + 0x804000)	/* BSC1 I2C/TWI */
#define BCM2835_USB_BASE	(BCM2835_PERI_BASE + 0x980000)	/* DTC_OTG USB controller */
#define BCM2835_MCORE_BASE	(BCM2835_PERI_BASE + 0x0000)	/* Fake frame buffer device (actually the multicore sync block*/

#endif

/* END */
