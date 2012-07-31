/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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

/* S3C2410 device base addresses */
#define S3C_MEMCTL_BASE			0x48000000
#define S3C2410_USB_HOST_BASE		0x49000000
#define S3C2410_INTERRUPT_BASE		0x4A000000
#define S3C2410_DMA_BASE		0x4B000000
#define S3C_CLOCK_POWER_BASE		0x4C000000
#define S3C2410_LCD_BASE		0x4D000000
#define S3C24X0_NAND_BASE		0x4E000000
#define S3C_UART_BASE			0x50000000
#define S3C_TIMER_BASE			0x51000000
#define S3C2410_USB_DEVICE_BASE		0x52000140
#define S3C_WATCHDOG_BASE		0x53000000
#define S3C2410_I2C_BASE		0x54000000
#define S3C2410_I2S_BASE		0x55000000
#define S3C_GPIO_BASE			0x56000000
#define S3C2410_RTC_BASE		0x57000000
#define S3C2410_ADC_BASE		0x58000000
#define S3C2410_SPI_BASE		0x59000000
#define S3C2410_SDI_BASE		0x5A000000

/* external IO space */
#define S3C_CS0_BASE 0x00000000
#define S3C_CS1_BASE 0x08000000
#define S3C_CS2_BASE 0x10000000
#define S3C_CS3_BASE 0x18000000
#define S3C_CS4_BASE 0x20000000
#define S3C_CS5_BASE 0x28000000
#define S3C_CS6_BASE 0x30000000

#define S3C_SDRAM_BASE S3C_CS6_BASE
#define S3C_SDRAM_END (S3C_SDRAM_BASE + 0x10000000)

/*
 * if we are booting from NAND, its internal SRAM occures at
 * a different address than without this feature
 */
#ifdef CONFIG_S3C_NAND_BOOT
# define NFC_RAM_AREA 0x00000000
#else
# define NFC_RAM_AREA 0x40000000
#endif
#define NFC_RAM_SIZE 4096

#define S3C_UART1_BASE (S3C_UART_BASE)
#define S3C_UART1_SIZE 0x4000
#define S3C_UART2_BASE (S3C_UART_BASE + 0x4000)
#define S3C_UART2_SIZE 0x4000
#define S3C_UART3_BASE (S3C_UART_BASE + 0x8000)
#define S3C_UART3_SIZE 0x4000
