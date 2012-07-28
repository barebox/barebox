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

#ifndef INCLUDE_MACH_DEVICES_S3C64XX_H
# define INCLUDE_MACH_DEVICES_S3C64XX_H

#include <driver.h>
#include <mach/s3c64xx-iomap.h>

static inline void s3c64xx_add_uart1(void)
{
	add_generic_device("s3c_serial", DEVICE_ID_DYNAMIC, NULL, S3C_UART1_BASE,
			S3C_UART1_SIZE, IORESOURCE_MEM, NULL);
}

static inline void s3c64xx_add_uart2(void)
{
	add_generic_device("s3c_serial", DEVICE_ID_DYNAMIC, NULL, S3C_UART2_BASE,
			S3C_UART2_SIZE, IORESOURCE_MEM, NULL);
}

static inline void s3c64xx_add_uart3(void)
{
	add_generic_device("s3c_serial", DEVICE_ID_DYNAMIC, NULL, S3C_UART3_BASE,
			S3C_UART3_SIZE, IORESOURCE_MEM, NULL);
}

#endif /* INCLUDE_MACH_DEVICES_S3C64XX_H */
