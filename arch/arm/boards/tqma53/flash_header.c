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
#include <asm/barebox-arm-head.h>
#include <mach/imx-flash-header.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

struct imx_dcd_v2_entry __dcd_entry_section dcd_entry[] = {
	/* IOMUX */
	{ .addr = cpu_to_be32(0x53fa8554), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8558), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8560), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8564), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8568), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8570), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8574), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8578), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa857c), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8580), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8584), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8588), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8590), .val = cpu_to_be32(0x00300040), },
	{ .addr = cpu_to_be32(0x53fa8594), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa86f0), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa86f4), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa86fc), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8714), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8718), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa871c), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8720), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa8724), .val = cpu_to_be32(0x04000000), },
	{ .addr = cpu_to_be32(0x53fa8728), .val = cpu_to_be32(0x00300000), },
	{ .addr = cpu_to_be32(0x53fa872c), .val = cpu_to_be32(0x00300000), },
	/* ESDCTL */
	{ .addr = cpu_to_be32(0x63fd9088), .val = cpu_to_be32(0x35343535), },
	{ .addr = cpu_to_be32(0x63fd9090), .val = cpu_to_be32(0x4d444c44), },
	{ .addr = cpu_to_be32(0x63fd907c), .val = cpu_to_be32(0x01370138), },
	{ .addr = cpu_to_be32(0x63fd9080), .val = cpu_to_be32(0x013b013c), },
	{ .addr = cpu_to_be32(0x63fd90f8), .val = cpu_to_be32(0x00000800), },
#ifdef CONFIG_MACH_TQMA53_1GB_RAM
	/* sync with u-boot: add WALAT for 4 chip variant */
	{ .addr = cpu_to_be32(0x63fd9018), .val = cpu_to_be32(0x00011740), },
	{ .addr = cpu_to_be32(0x63fd9000), .val = cpu_to_be32(0xc3190000), },
#else
	{ .addr = cpu_to_be32(0x63fd9018), .val = cpu_to_be32(0x00101740), },
	{ .addr = cpu_to_be32(0x63fd9000), .val = cpu_to_be32(0x83190000), },
#endif
	{ .addr = cpu_to_be32(0x63fd900c), .val = cpu_to_be32(0x9f5152e3), },
	{ .addr = cpu_to_be32(0x63fd9010), .val = cpu_to_be32(0xb68e8a63), },
	{ .addr = cpu_to_be32(0x63fd9014), .val = cpu_to_be32(0x01ff00db), },
	{ .addr = cpu_to_be32(0x63fd902c), .val = cpu_to_be32(0x000026d2), },
	/* Engcm12377 / errata sheet 03/2013 */
	{ .addr = cpu_to_be32(0x63fd9030), .val = cpu_to_be32(0x009f0e23), },
	{ .addr = cpu_to_be32(0x63fd9008), .val = cpu_to_be32(0x12273030), },
	{ .addr = cpu_to_be32(0x63fd9004), .val = cpu_to_be32(0x0002002d), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00008032), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00008033), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00028031), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x052080b0), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x04008040), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x0000803a), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x0000803b), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00028039), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x05208138), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x04008048), },
	{ .addr = cpu_to_be32(0x63fd9020), .val = cpu_to_be32(0x00005800), },
	/* prevent reserved value, use default TZQ_CS */
	{ .addr = cpu_to_be32(0x63fd9040), .val = cpu_to_be32(0x05380003), },
	{ .addr = cpu_to_be32(0x63fd9058), .val = cpu_to_be32(0x00022227), },
	{ .addr = cpu_to_be32(0x63fd901C), .val = cpu_to_be32(0x00000000), },
};

#define APP_DEST	0x70000000

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
	.dcd.command.length	= cpu_to_be16(sizeof(struct imx_dcd_command) + sizeof(dcd_entry)),
	.dcd.command.param	= DCD_COMMAND_WRITE_PARAM,
};
