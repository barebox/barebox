/*
 * Copyright (C) 2009 Juergen Beisert
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

/**
 * @file
 * @brief a9m2410dev Baseboad specific initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

#include "baseboards.h"

/**
 * Initialize the CPU to be able to work with the a9m2410dev evaluation board
 */
int a9m2410dev_devices_init(void)
{
	unsigned int reg;

	/* ---------- configure the GPIOs ------------- */
	writel(0x007FFFFF, S3C_GPACON);
	writel(0x00000000, S3C_GPCCON);
	writel(0x00000000, S3C_GPCUP);
	writel(0x00000000, S3C_GPDCON);
	writel(0x00000000, S3C_GPDUP);
	writel(0xAAAAAAAA, S3C_GPECON);
	writel(0x0000E03F, S3C_GPEUP);
	writel(0x00000000, S3C_GPBCON);	/* all inputs */
	writel(0x00000007, S3C_GPBUP);	/* pullup disabled for GPB0..3 */
	writel(0x00009000, S3C_GPFCON);	/* GPF7 CLK_INT#, GPF6 Debug-LED */
	writel(0x000000FF, S3C_GPFUP);
	writel(readl(S3C_GPGDAT) | 0x1010, S3C_GPGDAT);	/* switch off IDLE_SW#, switch off LCD backlight */
	writel(0x0100A93A, S3C_GPGCON);	/* switch on USB device */
	writel(0x0000F000, S3C_GPGUP);
	writel(0x0029FAAA, S3C_GPHCON);

	writel((1 << 12) | (0 << 11), S3C_GPJDAT);
	writel(0x0016aaaa, S3C_GPJCON);
	writel(~((0<<12)| (1<<11)), S3C_GPJUP);

	writel((0 << 12) | (0 << 11), S3C_GPJDAT);
	writel(0x0016aaaa, S3C_GPJCON);
	writel(0x00001fff, S3C_GPJUP);

	writel(0x00000000, S3C_DSC0);
	writel(0x00000000, S3C_DSC1);

	/*
	 * USB port1 normal, USB port0 normal, USB1 pads for device
	 * PCLK output on CLKOUT0, UPLL CLK output on CLKOUT1,
	 */
	writel((readl(S3C_MISCCR) & ~0xFFFF) | 0x0140, S3C_MISCCR);

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(S3C_BWSCON);

	/* CS#1 to access the network controller */
	reg &= ~0xf0;
	reg |= 0xe0;
	writel(0x1350, S3C_BANKCON1);

	/* CS#2 to the dual 16550 UART */
	reg &= ~0xf00;
	reg |= 0x400;
	writel(0x0d50, S3C_BANKCON2);

	writel(reg, S3C_BWSCON);

	/* release the reset signal to the network and UART device */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	return 0;
}
