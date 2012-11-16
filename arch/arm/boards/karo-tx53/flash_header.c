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
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_imx_fcb_head();
}

/*
 * FIXME: These are the dcd values for a Ka-Ro TX53 1011 which
 *        is not in production. It has 1GB DDR2 memory.
 */
#ifdef CONFIG_TX53_REV_1011

#define DCD_NAME_1011 struct imx_dcd_v2_entry __dcd_entry_section dcd_entry

#include "dcd-data-1011.h"

#elif defined(CONFIG_TX53_REV_XX30)

#define DCD_NAME_XX30 u32 __dcd_entry_section dcd_entry

#include "dcd-data-xx30.h"

#endif

#define APP_DEST	0x71000000

int tx53_dcdentry_size = sizeof(dcd_entry);
void *tx53_dcd_entry = &dcd_entry;

struct imx_flash_header_v2 __flash_header_section flash_header = {
	.header.tag		= IVT_HEADER_TAG,
	.header.length		= cpu_to_be16(32),
	.header.version		= IVT_VERSION,

	.entry			= APP_DEST + 0x1000,
	.dcd_ptr		= APP_DEST + 0x400 + offsetof(struct imx_flash_header_v2, dcd),
	.boot_data_ptr		= APP_DEST + 0x400 + offsetof(struct imx_flash_header_v2, boot_data),
	.self			= APP_DEST + 0x400,

	.boot_data.start	= APP_DEST,
	.boot_data.size		= DCD_BAREBOX_SIZE,

	.dcd.header.tag		= DCD_HEADER_TAG,
	.dcd.header.length	= cpu_to_be16(sizeof(struct imx_dcd) + sizeof(dcd_entry)),
	.dcd.header.version	= DCD_VERSION,

	.dcd.command.tag	= DCD_COMMAND_WRITE_TAG,
#ifdef CONFIG_TX53_REV_1011
	.dcd.command.length	= cpu_to_be16(sizeof(struct imx_dcd_command) + sizeof(dcd_entry)),
#elif defined(CONFIG_TX53_REV_XX30)
	.dcd.command.length	= cpu_to_be16(0x21c),
#endif
	.dcd.command.param	= DCD_COMMAND_WRITE_PARAM,
};
