/*
 * Copyright (C) 2012 Alexey Galakhov
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
 *
 */

/* S5PV210 device base addresses */

#define S5P_CLOCK_POWER_BASE	0xE0100000
#define S3C_GPIO_BASE		0xE0200000
#define S3C_TIMER_BASE		0xE2500000
#define S3C_WATCHDOG_BASE	0xE2700000
#define S3C_UART_BASE		0xE2900000
#define S3C_USB_HOST_BASE	0xEC200000
#define S3C_NAND_BASE		0xB0E00000

/* external IO space */
#define S3C_CS0_BASE 0x80000000
#define S3C_CS1_BASE 0x88000000
#define S3C_CS2_BASE 0x90000000
#define S3C_CS3_BASE 0x98000000
#define S3C_CS4_BASE 0xA0000000
#define S3C_CS5_BASE 0xA8000000

#define S3C_SDRAM_BASE 0x20000000
#define S3C_SDRAM_END (S3C_SDRAM_BASE + 0x60000000)

#define S3C_UART1_BASE (S3C_UART_BASE)
#define S3C_UART1_SIZE 0x400
#define S3C_UART2_BASE (S3C_UART_BASE + 0x400)
#define S3C_UART2_SIZE 0x400
#define S3C_UART3_BASE (S3C_UART_BASE + 0x800)
#define S3C_UART3_SIZE 0x400

#define S5P_DMC0_BASE 0xF0000000
#define S5P_DMC1_BASE 0xF1400000
