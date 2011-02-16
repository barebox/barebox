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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/* S3C2410 device base addresses */
#define S3C24X0_SDRAM_BASE		0x30000000
#define S3C24X0_SDRAM_END		0x40000000
#define S3C24X0_MEMCTL_BASE		0x48000000
#define S3C2410_USB_HOST_BASE		0x49000000
#define S3C2410_INTERRUPT_BASE		0x4A000000
#define S3C2410_DMA_BASE		0x4B000000
#define S3C24X0_CLOCK_POWER_BASE	0x4C000000
#define S3C2410_LCD_BASE		0x4D000000
#define S3C24X0_NAND_BASE		0x4E000000
#define S3C24X0_UART_BASE		0x50000000
#define S3C24X0_TIMER_BASE		0x51000000
#define S3C2410_USB_DEVICE_BASE		0x52000140
#define S3C24X0_WATCHDOG_BASE		0x53000000
#define S3C2410_I2C_BASE		0x54000000
#define S3C2410_I2S_BASE		0x55000000
#define S3C24X0_GPIO_BASE		0x56000000
#define S3C2410_RTC_BASE		0x57000000
#define S3C2410_ADC_BASE		0x58000000
#define S3C2410_SPI_BASE		0x59000000
#define S3C2410_SDI_BASE		0x5A000000

/* Clock control (direct access) */

#define LOCKTIME (S3C24X0_CLOCK_POWER_BASE)
#define MPLLCON (S3C24X0_CLOCK_POWER_BASE + 0x4)
#define UPLLCON (S3C24X0_CLOCK_POWER_BASE + 0x8)
#define CLKCON (S3C24X0_CLOCK_POWER_BASE + 0xc)
#define CLKSLOW (S3C24X0_CLOCK_POWER_BASE + 0x10)
#define CLKDIVN (S3C24X0_CLOCK_POWER_BASE + 0x14)

/* Timer (direct access) */
#define TCFG0 (S3C24X0_TIMER_BASE + 0x00)
#define TCFG1 (S3C24X0_TIMER_BASE + 0x04)
#define TCON (S3C24X0_TIMER_BASE + 0x08)
#define TCNTB0 (S3C24X0_TIMER_BASE + 0x0c)
#define TCMPB0 (S3C24X0_TIMER_BASE + 0x10)
#define TCNTO0 (S3C24X0_TIMER_BASE + 0x14)
#define TCNTB1 (S3C24X0_TIMER_BASE + 0x18)
#define TCMPB1 (S3C24X0_TIMER_BASE + 0x1c)
#define TCNTO1 (S3C24X0_TIMER_BASE + 0x20)
#define TCNTB2 (S3C24X0_TIMER_BASE + 0x24)
#define TCMPB2 (S3C24X0_TIMER_BASE + 0x28)
#define TCNTO2 (S3C24X0_TIMER_BASE + 0x2c)
#define TCNTB3 (S3C24X0_TIMER_BASE + 0x30)
#define TCMPB3 (S3C24X0_TIMER_BASE + 0x34)
#define TCNTO3 (S3C24X0_TIMER_BASE + 0x38)
#define TCNTB4 (S3C24X0_TIMER_BASE + 0x3c)
#define TCNTO4 (S3C24X0_TIMER_BASE + 0x40)

/* Watchdog (direct access) */
#define WTCON (S3C24X0_WATCHDOG_BASE)
#define WTDAT (S3C24X0_WATCHDOG_BASE + 0x04)
#define WTCNT (S3C24X0_WATCHDOG_BASE + 0x08)

/*
 * if we are booting from NAND, its internal SRAM occures at
 * a different address than without this feature
 */
#ifdef CONFIG_S3C24XX_NAND_BOOT
# define NFC_RAM_AREA 0x00000000
#else
# define NFC_RAM_AREA 0x40000000
#endif
#define NFC_RAM_SIZE 4096

/* internal UARTs (driver based) */
#define UART1_BASE (S3C24X0_UART_BASE)
#define UART1_SIZE 0x4000
#define UART2_BASE (S3C24X0_UART_BASE + 0x4000)
#define UART2_SIZE 0x4000
#define UART3_BASE (S3C24X0_UART_BASE + 0x8000)
#define UART3_SIZE 0x4000

/* CS configuration (direct access) */
#define BWSCON (S3C24X0_MEMCTL_BASE)
#define BANKCON0 (S3C24X0_MEMCTL_BASE + 0x04)
#define BANKCON1 (S3C24X0_MEMCTL_BASE + 0x08)
#define BANKCON2 (S3C24X0_MEMCTL_BASE + 0x0c)
#define BANKCON3 (S3C24X0_MEMCTL_BASE + 0x10)
#define BANKCON4 (S3C24X0_MEMCTL_BASE + 0x14)
#define BANKCON5 (S3C24X0_MEMCTL_BASE + 0x18)
#define BANKCON6 (S3C24X0_MEMCTL_BASE + 0x1c)
#define BANKCON7 (S3C24X0_MEMCTL_BASE + 0x20)
#define REFRESH (S3C24X0_MEMCTL_BASE + 0x24)
#define BANKSIZE (S3C24X0_MEMCTL_BASE + 0x28)
#define MRSRB6 (S3C24X0_MEMCTL_BASE + 0x2c)
#define MRSRB7 (S3C24X0_MEMCTL_BASE + 0x30)

/* GPIO registers (direct access) */
#define GPACON (S3C24X0_GPIO_BASE)
#define GPADAT (S3C24X0_GPIO_BASE + 0x04)

#define GPBCON (S3C24X0_GPIO_BASE + 0x10)
#define GPBDAT (S3C24X0_GPIO_BASE + 0x14)
#define GPBUP (S3C24X0_GPIO_BASE + 0x18)

#define GPCCON (S3C24X0_GPIO_BASE + 0x20)
#define GPCDAT (S3C24X0_GPIO_BASE + 0x24)
#define GPCUP (S3C24X0_GPIO_BASE + 0x28)

#define GPDCON (S3C24X0_GPIO_BASE + 0x30)
#define GPDDAT (S3C24X0_GPIO_BASE + 0x34)
#define GPDUP (S3C24X0_GPIO_BASE + 0x38)

#define GPECON (S3C24X0_GPIO_BASE + 0x40)
#define GPEDAT (S3C24X0_GPIO_BASE + 0x44)
#define GPEUP (S3C24X0_GPIO_BASE + 0x48)

#define GPFCON (S3C24X0_GPIO_BASE + 0x50)
#define GPFDAT (S3C24X0_GPIO_BASE + 0x54)
#define GPFUP (S3C24X0_GPIO_BASE + 0x58)

#define GPGCON (S3C24X0_GPIO_BASE + 0x60)
#define GPGDAT (S3C24X0_GPIO_BASE + 0x64)
#define GPGUP (S3C24X0_GPIO_BASE + 0x68)

#define GPHCON (S3C24X0_GPIO_BASE + 0x70)
#define GPHDAT (S3C24X0_GPIO_BASE + 0x74)
#define GPHUP (S3C24X0_GPIO_BASE + 0x78)

#ifdef CONFIG_CPU_S3C2440
# define GPJCON (S3C24X0_GPIO_BASE + 0xd0)
# define GPJDAT (S3C24X0_GPIO_BASE + 0xd4)
# define GPJUP (S3C24X0_GPIO_BASE + 0xd8)
#endif

#define MISCCR  (S3C24X0_GPIO_BASE + 0x80)
#define DCLKCON (S3C24X0_GPIO_BASE + 0x84)
#define EXTINT0 (S3C24X0_GPIO_BASE + 0x88)
#define EXTINT1 (S3C24X0_GPIO_BASE + 0x8c)
#define EXTINT2 (S3C24X0_GPIO_BASE + 0x90)
#define EINTFLT0 (S3C24X0_GPIO_BASE + 0x94)
#define EINTFLT1 (S3C24X0_GPIO_BASE + 0x98)
#define EINTFLT2 (S3C24X0_GPIO_BASE + 0x9c)
#define EINTFLT3 (S3C24X0_GPIO_BASE + 0xa0)
#define EINTMASK (S3C24X0_GPIO_BASE + 0xa4)
#define EINTPEND (S3C24X0_GPIO_BASE + 0xa8)
#define GSTATUS0 (S3C24X0_GPIO_BASE + 0xac)
#define GSTATUS1 (S3C24X0_GPIO_BASE + 0xb0)
#define GSTATUS2 (S3C24X0_GPIO_BASE + 0xb4)
#define GSTATUS3 (S3C24X0_GPIO_BASE + 0xb8)
#define GSTATUS4 (S3C24X0_GPIO_BASE + 0xbc)

#ifdef CONFIG_CPU_S3C2440
# define DSC0 (S3C24X0_GPIO_BASE + 0xc4)
# define DSC1 (S3C24X0_GPIO_BASE + 0xc8)
#endif

/* external IO space */
#define CS0_BASE 0x00000000
#define CS1_BASE 0x08000000
#define CS2_BASE 0x10000000
#define CS3_BASE 0x18000000
#define CS4_BASE 0x20000000
#define CS5_BASE 0x28000000
#define CS6_BASE 0x30000000
