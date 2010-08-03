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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <asm/io.h>
#include <mach/s3c24x0-iomap.h>

/**
 * Initialize the CPU to be able to work with the a9m2410dev evaluation board
 */
int a9m2410dev_devices_init(void)
{
	unsigned int reg;

	/* ---------- configure the GPIOs ------------- */
	writel(0x007FFFFF, GPACON);
	writel(0x00000000, GPCCON);
	writel(0x00000000, GPCUP);
	writel(0x00000000, GPDCON);
	writel(0x00000000, GPDUP);
	writel(0xAAAAAAAA, GPECON);
	writel(0x0000E03F, GPEUP);
	writel(0x00000000, GPBCON);	/* all inputs */
	writel(0x00000007, GPBUP);	/* pullup disabled for GPB0..3 */
	writel(0x00009000, GPFCON);	/* GPF7 CLK_INT#, GPF6 Debug-LED */
	writel(0x000000FF, GPFUP);
	writel(readl(GPGDAT) | 0x1010, GPGDAT);	/* switch off IDLE_SW#, switch off LCD backlight */
	writel(0x0100A93A, GPGCON);	/* switch on USB device */
	writel(0x0000F000, GPGUP);
	writel(0x0029FAAA, GPHCON);

	writel((1 << 12) | (0 << 11), GPJDAT);
	writel(0x0016aaaa, GPJCON);
	writel(~((0<<12)| (1<<11)), GPJUP);

	writel((0 << 12) | (0 << 11), GPJDAT);
	writel(0x0016aaaa, GPJCON);
	writel(0x00001fff, GPJUP);

	writel(0x00000000, DSC0);
	writel(0x00000000, DSC1);

	/*
	 * USB port1 normal, USB port0 normal, USB1 pads for device
	 * PCLK output on CLKOUT0, UPLL CLK output on CLKOUT1,
	 */
	writel((readl(MISCCR) & ~0xFFFF) | 0x0140, MISCCR);

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(BWSCON);

	/* CS#1 to access the network controller */
	reg &= ~0xf0;
	reg |= 0xe0;
	writel(0x1350, BANKCON1);

	/* CS#2 to the dual 16550 UART */
	reg &= ~0xf00;
	reg |= 0x400;
	writel(0x0d50, BANKCON2);

	writel(reg, BWSCON);

	/* release the reset signal to the network and UART device */
        reg = readl(MISCCR);
	reg |= 0x10000;
	writel(reg, MISCCR);

	return 0;
}
