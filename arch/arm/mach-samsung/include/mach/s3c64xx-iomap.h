/*
 * Copyright (C) 2012 Juergen Beisert
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
 */

/* S3C64xx device base addresses */
#define S3C_SROM_SFR		0x70000000
#define S3C_NAND_BASE		0x70200000
#define S3C_SDI0_BASE		0x7c200000
#define S3C_SDI0_SIZE		0x100
#define S3C_SDI1_BASE		0x7c300000
#define S3C_SDI1_SIZE		0x100
#define S3C_SDI2_BASE		0x7c400000
#define S3C_SDI2_SIZE		0x100
#define S3C_DRAMC		0x7e001000
#define S3C_WATCHDOG_BASE	0x7e004000
#define S3C_CLOCK_POWER_BASE	0x7e00f000
#define S3C_UART_BASE		0x7f005000
#define S3C_TIMER_BASE		0x7f006000
#define S3C_GPIO_BASE		0x7f008000

#define S3C_UART1_BASE (S3C_UART_BASE)
#define S3C_UART1_SIZE 0x400
#define S3C_UART2_BASE (S3C_UART_BASE + 0x400)
#define S3C_UART2_SIZE 0x400
#define S3C_UART3_BASE (S3C_UART_BASE + 0x800)
#define S3C_UART3_SIZE 0x400
#define S3C_UART4_BASE (S3C_UART_BASE + 0xc00)
#define S3C_UART4_SIZE 0x400

#define S3C_SDRAM_BASE 0x50000000
#define S3C_SDRAM_END (S3C_SDRAM_BASE + 0x10000000)

#define S3C_SROM_BW (S3C_SROM_SFR)
#define S3C_SROM_BC0 (S3C_SROM_SFR + 4)

#define S3C_CS0_BASE 0x10000000
#define S3C_CS1_BASE 0x18000000
#define S3C_CS2_BASE 0x20000000
#define S3C_CS3_BASE 0x28000000
#define S3C_CS4_BASE 0x30000000
#define S3C_CS5_BASE 0x38000000
