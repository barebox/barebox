/*
 * (C) Copyright 2003
 * David Müller ELSOFT AG Switzerland. d.mueller@elsoft.ch
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

/************************************************
 * NAME	    : s3c2400.h
 * Version  : 31.3.2003
 *
 * Based on S3C2400X User's manual Rev 1.1
 ************************************************/

#ifndef __S3C2400_H__
#define __S3C2400_H__

#define S3C24X0_UART_CHANNELS	2
#define S3C24X0_SPI_CHANNELS	1
#define PALETTE			(0x14A00400)	/* SJS */

typedef enum {
	S3C24X0_UART0,
	S3C24X0_UART1,
} S3C24X0_UARTS_NR;

/* S3C2400 device base addresses */
#define S3C24X0_MEMCTL_BASE		0x14000000
#define S3C24X0_USB_HOST_BASE		0x14200000
#define S3C24X0_INTERRUPT_BASE		0x14400000
#define S3C24X0_DMA_BASE		0x14600000
#define S3C24X0_CLOCK_POWER_BASE	0x14800000
#define S3C24X0_LCD_BASE		0x14A00000
#define S3C24X0_UART_BASE		0x15000000
#define S3C24X0_TIMER_BASE		0x15100000
#define S3C24X0_USB_DEVICE_BASE		0x15200140
#define S3C24X0_WATCHDOG_BASE		0x15300000
#define S3C24X0_I2C_BASE		0x15400000
#define S3C24X0_I2S_BASE		0x15508000
#define S3C24X0_GPIO_BASE		0x15600000
#define S3C24X0_RTC_BASE		0x15700000
#define S3C24X0_ADC_BASE		0x15800000
#define S3C24X0_SPI_BASE		0x15900000
#define S3C2400_MMC_BASE		0x15A00000

/* include common stuff */
#include <s3c24x0.h>


static inline S3C24X0_MEMCTL * S3C24X0_GetBase_MEMCTL(void)
{
	return (S3C24X0_MEMCTL * const)S3C24X0_MEMCTL_BASE;
}
static inline S3C24X0_USB_HOST * S3C24X0_GetBase_USB_HOST(void)
{
	return (S3C24X0_USB_HOST * const)S3C24X0_USB_HOST_BASE;
}
static inline S3C24X0_INTERRUPT * S3C24X0_GetBase_INTERRUPT(void)
{
	return (S3C24X0_INTERRUPT * const)S3C24X0_INTERRUPT_BASE;
}
static inline S3C24X0_DMAS * S3C24X0_GetBase_DMAS(void)
{
	return (S3C24X0_DMAS * const)S3C24X0_DMA_BASE;
}
static inline S3C24X0_CLOCK_POWER * S3C24X0_GetBase_CLOCK_POWER(void)
{
	return (S3C24X0_CLOCK_POWER * const)S3C24X0_CLOCK_POWER_BASE;
}
static inline S3C24X0_LCD * S3C24X0_GetBase_LCD(void)
{
	return (S3C24X0_LCD * const)S3C24X0_LCD_BASE;
}
static inline S3C24X0_UART * S3C24X0_GetBase_UART(S3C24X0_UARTS_NR nr)
{
	return (S3C24X0_UART * const)(S3C24X0_UART_BASE + (nr * 0x4000));
}
static inline S3C24X0_TIMERS * S3C24X0_GetBase_TIMERS(void)
{
	return (S3C24X0_TIMERS * const)S3C24X0_TIMER_BASE;
}
static inline S3C24X0_USB_DEVICE * S3C24X0_GetBase_USB_DEVICE(void)
{
	return (S3C24X0_USB_DEVICE * const)S3C24X0_USB_DEVICE_BASE;
}
static inline S3C24X0_WATCHDOG * S3C24X0_GetBase_WATCHDOG(void)
{
	return (S3C24X0_WATCHDOG * const)S3C24X0_WATCHDOG_BASE;
}
static inline S3C24X0_I2C * S3C24X0_GetBase_I2C(void)
{
	return (S3C24X0_I2C * const)S3C24X0_I2C_BASE;
}
static inline S3C24X0_I2S * S3C24X0_GetBase_I2S(void)
{
	return (S3C24X0_I2S * const)S3C24X0_I2S_BASE;
}
static inline S3C24X0_GPIO * S3C24X0_GetBase_GPIO(void)
{
	return (S3C24X0_GPIO * const)S3C24X0_GPIO_BASE;
}
static inline S3C24X0_RTC * S3C24X0_GetBase_RTC(void)
{
	return (S3C24X0_RTC * const)S3C24X0_RTC_BASE;
}
static inline S3C2400_ADC * S3C2400_GetBase_ADC(void)
{
	return (S3C2400_ADC * const)S3C24X0_ADC_BASE;
}
static inline S3C24X0_SPI * S3C24X0_GetBase_SPI(void)
{
	return (S3C24X0_SPI * const)S3C24X0_SPI_BASE;
}
static inline S3C2400_MMC * S3C2400_GetBase_MMC(void)
{
	return (S3C2400_MMC * const)S3C2400_MMC_BASE;
}

#endif /*__S3C2400_H__*/
