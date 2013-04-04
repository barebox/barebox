/*
 * Copyright (C) 2011 Marc Kleine-Budde <mkl@pengutronix.de>
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

#include <common.h>
#include <asm/byteorder.h>
#include <mach/imx-flash-header.h>
#include <mach/imx6-regs.h>
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

#define APP_DEST	0x00907000

struct imx_flash_header_v2 __flash_header_section flash_header = {
	.header.tag		= IVT_HEADER_TAG,
	.header.length		= cpu_to_be16(32),
	.header.version		= IVT_VERSION,
	.entry			= APP_DEST + 0x2000,
	.dcd_ptr		= 0,
	.boot_data_ptr		= APP_DEST + FLASH_HEADER_OFFSET + offsetof(struct imx_flash_header_v2, boot_data),
	.self			= APP_DEST + FLASH_HEADER_OFFSET,

	.boot_data.start	= APP_DEST,
	.boot_data.size		= barebox_image_size,
};
