/*
 * Copyright 2012 Juergen Beisert
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
 */

#ifndef INCLUDE_MACH_DEVICES_S3C24XX_H
# define INCLUDE_MACH_DEVICES_S3C24XX_H

#include <driver.h>
#include <mach/s3c24xx-iomap.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-mci.h>
#include <mach/s3c24xx-fb.h>

static inline void s3c24xx_add_nand(struct s3c24x0_nand_platform_data *d)
{
	add_generic_device("s3c24x0_nand", DEVICE_ID_DYNAMIC, NULL,
				S3C24X0_NAND_BASE, 0x80, IORESOURCE_MEM, d);
}

static inline void s3c24xx_add_mci(struct s3c_mci_platform_data *d)
{
	add_generic_device("s3c_mci", DEVICE_ID_DYNAMIC, NULL,
				S3C2410_SDI_BASE, 0x80, IORESOURCE_MEM, d);
}

static inline void s3c24xx_add_fb(struct s3c_fb_platform_data *d)
{
	add_generic_device("s3c_fb", DEVICE_ID_DYNAMIC, NULL,
				S3C2410_LCD_BASE, 0x80, IORESOURCE_MEM, d);
}

static inline void s3c24xx_add_ohci(void)
{
	add_generic_device("ohci", DEVICE_ID_DYNAMIC, NULL,
			S3C2410_USB_HOST_BASE, 0x100, IORESOURCE_MEM, NULL);
}

static inline void s3c24xx_add_uart1(void)
{
	add_generic_device("s3c_serial", DEVICE_ID_DYNAMIC, NULL, S3C_UART1_BASE,
			S3C_UART1_SIZE, IORESOURCE_MEM, NULL);
}

#endif /* INCLUDE_MACH_DEVICES_S3C24XX_H */
