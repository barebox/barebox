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
struct imx_dcd_v2_entry __dcd_entry_section dcd_entry[] = {
	{ .addr = cpu_to_be32(0x53fd406c), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd4070), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd4074), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd4078), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd407c), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd4080), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fd4088), .val = cpu_to_be32(0xffffffff), },
	{ .addr = cpu_to_be32(0x53fa8174), .val = cpu_to_be32(0x00000011), },
	{ .addr = cpu_to_be32(0x63fd800c), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8554), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8560), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8594), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8584), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8558), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa8568), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa8590), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa857c), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa8564), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa8580), .val = cpu_to_be32(0x00200040), },
	{ .addr = cpu_to_be32(0x53fa8570), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8578), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa872c), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8728), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa871c), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8718), .val = cpu_to_be32(0x00200000), },
	{ .addr = cpu_to_be32(0x53fa8574), .val = cpu_to_be32(0x00280000), },
	{ .addr = cpu_to_be32(0x53fa8588), .val = cpu_to_be32(0x00280000), },
	{ .addr = cpu_to_be32(0x53fa86f0), .val = cpu_to_be32(0x00280000), },
	{ .addr = cpu_to_be32(0x53fa8720), .val = cpu_to_be32(0x00280000), },
	{ .addr = cpu_to_be32(0x53fa86fc), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa86f4), .val = cpu_to_be32(0x00000200), },
	{ .addr = cpu_to_be32(0x53fa8714), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8724), .val = cpu_to_be32(0x06000000), },
	{ .addr = cpu_to_be32(0x63fd9088), .val = cpu_to_be32(0x36353b38), },
	{ .addr = cpu_to_be32(0x63fd9090), .val = cpu_to_be32(0x49434942), },
	{ .addr = cpu_to_be32(0x63fd90f8), .val = cpu_to_be32(0x00000800), },
	{ .addr = cpu_to_be32(0x63fd907c), .val = cpu_to_be32(0x01350138), },
	{ .addr = cpu_to_be32(0x63fd9080), .val = cpu_to_be32(0x01380139), },
	{ .addr = cpu_to_be32(0x63fd9018), .val = cpu_to_be32(0x00001710), },
	{ .addr = cpu_to_be32(0x63fd9000), .val = cpu_to_be32(0x84110000), },
	{ .addr = cpu_to_be32(0x63fd900c), .val = cpu_to_be32(0x4d5122d2), },
	{ .addr = cpu_to_be32(0x63fd9010), .val = cpu_to_be32(0xb6f18a22), },
	{ .addr = cpu_to_be32(0x63fd9014), .val = cpu_to_be32(0x00c700db), },
	{ .addr = cpu_to_be32(0x63fd902c), .val = cpu_to_be32(0x000026d2), },
	{ .addr = cpu_to_be32(0x63fd9030), .val = cpu_to_be32(0x009f000e), },
	{ .addr = cpu_to_be32(0x63fd9008), .val = cpu_to_be32(0x12272000), },
	{ .addr = cpu_to_be32(0x63fd9004), .val = cpu_to_be32(0x00030012), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x04008010), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00008020), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00008020), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x0a528030), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x03868031), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00068031), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00008032), },
	{ .addr = cpu_to_be32(0x63fd9020), .val = cpu_to_be32(0x00005800), },
	{ .addr = cpu_to_be32(0x63fd9058), .val = cpu_to_be32(0x00033332), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00448031), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x04008018), },
	{ .addr = cpu_to_be32(0x63fd901c), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x63fd9040), .val = cpu_to_be32(0x04b80003), },
	{ .addr = cpu_to_be32(0x53fa8004), .val = cpu_to_be32(0x00194005), },
	{ .addr = cpu_to_be32(0x53fa819c), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81a0), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81a4), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81a8), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81ac), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81b0), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81b4), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81b8), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81dc), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa81e0), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8228), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa822c), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8230), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8234), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa8238), .val = cpu_to_be32(0x00000000), },
	{ .addr = cpu_to_be32(0x53fa84ec), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa84f0), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa84f4), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa84f8), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa84fc), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa8500), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa8504), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa8508), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa852c), .val = cpu_to_be32(0x00000004), },
	{ .addr = cpu_to_be32(0x53fa8530), .val = cpu_to_be32(0x00000004), },
	{ .addr = cpu_to_be32(0x53fa85a0), .val = cpu_to_be32(0x00000004), },
	{ .addr = cpu_to_be32(0x53fa85a4), .val = cpu_to_be32(0x00000004), },
	{ .addr = cpu_to_be32(0x53fa85a8), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa85ac), .val = cpu_to_be32(0x000000e4), },
	{ .addr = cpu_to_be32(0x53fa85b0), .val = cpu_to_be32(0x00000004), },
};
#elif defined(CONFIG_TX53_REV_XX30)

#define DCD_ITEM(adr, val)	cpu_to_be32(adr), cpu_to_be32(val)
#define DCD_WR_CMD(len)		cpu_to_be32(0xcc << 24 | (len) << 8 | 0x04)
#define DCD_CHECK_CMD(a, b, c)	cpu_to_be32(a), cpu_to_be32(b), cpu_to_be32(c)

/*
 * This board uses advanced features of the DCD which do not corporate
 * well with our flash header defines. The DCD consists of commands which
 * have the length econded into them. Normally the DCDs only have a single
 * command (DCD_COMMAND_WRITE_TAG) which is already part of struct
 * imx_flash_header_v2. Now this board uses multiple commands, so we cannot
 * calculate the command length using sizeof(dcd_entry).
 */
u32 __dcd_entry_section dcd_entry[] = {
	DCD_ITEM(0x53fd4068, 0xffcc0fff),
	DCD_ITEM(0x53fd406c, 0x000fffc3),
	DCD_ITEM(0x53fd4070, 0x033c0000),
	DCD_ITEM(0x53fd4074, 0x00000000),
	DCD_ITEM(0x53fd4078, 0x00000000),
	DCD_ITEM(0x53fd407c, 0x00fff033),
	DCD_ITEM(0x53fd4080, 0x0f00030f),
	DCD_ITEM(0x53fd4084, 0xfff00000),
	DCD_ITEM(0x53fd4088, 0x00000000),
	DCD_ITEM(0x53fa8174, 0x00000011),
	DCD_ITEM(0x53fa8318, 0x00000011),
	DCD_ITEM(0x63fd800c, 0x00000000),
	DCD_ITEM(0x53fd4014, 0x00888944),
	DCD_ITEM(0x53fd4018, 0x00016154),
	DCD_ITEM(0x53fa8724, 0x04000000),
	DCD_ITEM(0x53fa86f4, 0x00000000),
	DCD_ITEM(0x53fa8714, 0x00000000),
	DCD_ITEM(0x53fa86fc, 0x00000080),
	DCD_ITEM(0x53fa8710, 0x00000000),
	DCD_ITEM(0x53fa8708, 0x00000040),
	DCD_ITEM(0x53fa8584, 0x00280000),
	DCD_ITEM(0x53fa8594, 0x00280000),
	DCD_ITEM(0x53fa8560, 0x00280000),
	DCD_ITEM(0x53fa8554, 0x00280000),
	DCD_ITEM(0x53fa857c, 0x00a80040),
	DCD_ITEM(0x53fa8590, 0x00a80040),
	DCD_ITEM(0x53fa8568, 0x00a80040),
	DCD_ITEM(0x53fa8558, 0x00a80040),
	DCD_ITEM(0x53fa8580, 0x00280040),
	DCD_ITEM(0x53fa8578, 0x00280000),
	DCD_ITEM(0x53fa8564, 0x00280040),
	DCD_ITEM(0x53fa8570, 0x00280000),
	DCD_ITEM(0x53fa858c, 0x000000c0),
	DCD_ITEM(0x53fa855c, 0x000000c0),
	DCD_ITEM(0x53fa8574, 0x00280000),
	DCD_ITEM(0x53fa8588, 0x00280000),
	DCD_ITEM(0x53fa86f0, 0x00280000),
	DCD_ITEM(0x53fa8720, 0x00280000),
	DCD_ITEM(0x53fa8718, 0x00280000),
	DCD_ITEM(0x53fa871c, 0x00280000),
	DCD_ITEM(0x53fa8728, 0x00280000),
	DCD_ITEM(0x53fa872c, 0x00280000),
	DCD_ITEM(0x63fd904c, 0x001f001f),
	DCD_ITEM(0x63fd9050, 0x001f001f),
	DCD_ITEM(0x63fd907c, 0x011e011e),
	DCD_ITEM(0x63fd9080, 0x011f0120),
	DCD_ITEM(0x63fd9088, 0x3a393d3b),
	DCD_ITEM(0x63fd9090, 0x3f3f3f3f),
	DCD_ITEM(0x63fd9018, 0x00011740),
	DCD_ITEM(0x63fd9000, 0x83190000),
	DCD_ITEM(0x63fd900c, 0x3f435316),
	DCD_ITEM(0x63fd9010, 0xb66e0a63),
	DCD_ITEM(0x63fd9014, 0x01ff00db),
	DCD_ITEM(0x63fd902c, 0x000026d2),
	DCD_ITEM(0x63fd9030, 0x00430f24),
	DCD_ITEM(0x63fd9008, 0x1b221010),
	DCD_ITEM(0x63fd9004, 0x00030012),
	DCD_ITEM(0x63fd901c, 0x00008032),
	DCD_ITEM(0x63fd901c, 0x00008033),
	DCD_ITEM(0x63fd901c, 0x00408031),
	DCD_ITEM(0x63fd901c, 0x055080b0),
	DCD_ITEM(0x63fd9020, 0x00005800),
	DCD_ITEM(0x63fd9058, 0x00011112),
	DCD_ITEM(0x63fd90d0, 0x00000003),
	DCD_ITEM(0x63fd901c, 0x04008010),
	DCD_ITEM(0x63fd901c, 0x00008040),
	DCD_ITEM(0x63fd9040, 0x0539002b),
	DCD_CHECK_CMD(0xcf000c04, 0x63fd9040, 0x00010000),
	DCD_WR_CMD(0x24),
	DCD_ITEM(0x63fd901c, 0x00048033),
	DCD_ITEM(0x63fd901c, 0x00848231),
	DCD_ITEM(0x63fd901c, 0x00000000),
	DCD_ITEM(0x63fd9048, 0x00000001),
	DCD_CHECK_CMD(0xcf000c04, 0x63fd9048, 0x00000001),
	DCD_WR_CMD(0x2c),
	DCD_ITEM(0x63fd901c, 0x00048031),
	DCD_ITEM(0x63fd901c, 0x00008033),
	DCD_ITEM(0x63fd901c, 0x04008010),
	DCD_ITEM(0x63fd901c, 0x00048033),
	DCD_ITEM(0x63fd907c, 0x90000000),
	DCD_CHECK_CMD(0xcf000c04, 0x63fd907c, 0x90000000),
	DCD_WR_CMD(0x2c),
	DCD_ITEM(0x63fd901c, 0x00008033),
	DCD_ITEM(0x63fd901c, 0x00000000),
	DCD_ITEM(0x63fd901c, 0x04008010),
	DCD_ITEM(0x63fd901c, 0x00048033),
	DCD_ITEM(0x63fd90a4, 0x00000010),
	DCD_CHECK_CMD(0xcf000c04, 0x63fd90a4, 0x00000010),
	DCD_WR_CMD(0x24),
	DCD_ITEM(0x63fd901c, 0x00008033),
	DCD_ITEM(0x63fd901c, 0x04008010),
	DCD_ITEM(0x63fd901c, 0x00048033),
	DCD_ITEM(0x63fd90a0, 0x00000010),
	DCD_CHECK_CMD(0xcf000c04, 0x63fd90a0, 0x00000010),
	DCD_WR_CMD(0x010c),
	DCD_ITEM(0x63fd901c, 0x00008033),
	DCD_ITEM(0x63fd901c, 0x00000000),
	DCD_ITEM(0x53fa8004, 0x00194005),
	DCD_ITEM(0x53fa819c, 0x00000000),
	DCD_ITEM(0x53fa81a0, 0x00000000),
	DCD_ITEM(0x53fa81a4, 0x00000000),
	DCD_ITEM(0x53fa81a8, 0x00000000),
	DCD_ITEM(0x53fa81ac, 0x00000000),
	DCD_ITEM(0x53fa81b0, 0x00000000),
	DCD_ITEM(0x53fa81b4, 0x00000000),
	DCD_ITEM(0x53fa81b8, 0x00000000),
	DCD_ITEM(0x53fa81dc, 0x00000000),
	DCD_ITEM(0x53fa81e0, 0x00000000),
	DCD_ITEM(0x53fa8228, 0x00000000),
	DCD_ITEM(0x53fa822c, 0x00000000),
	DCD_ITEM(0x53fa8230, 0x00000000),
	DCD_ITEM(0x53fa8234, 0x00000000),
	DCD_ITEM(0x53fa8238, 0x00000000),
	DCD_ITEM(0x53fa84ec, 0x000000e4),
	DCD_ITEM(0x53fa84f0, 0x000000e4),
	DCD_ITEM(0x53fa84f4, 0x000000e4),
	DCD_ITEM(0x53fa84f8, 0x000000e4),
	DCD_ITEM(0x53fa84fc, 0x000000e4),
	DCD_ITEM(0x53fa8500, 0x000000e4),
	DCD_ITEM(0x53fa8504, 0x000000e4),
	DCD_ITEM(0x53fa8508, 0x000000e4),
	DCD_ITEM(0x53fa852c, 0x00000004),
	DCD_ITEM(0x53fa8530, 0x00000004),
	DCD_ITEM(0x53fa85a0, 0x00000004),
	DCD_ITEM(0x53fa85a4, 0x00000004),
	DCD_ITEM(0x53fa85a8, 0x000000e4),
	DCD_ITEM(0x53fa85ac, 0x000000e4),
	DCD_ITEM(0x53fa85b0, 0x00000004),
};
#endif

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
#ifdef CONFIG_TX53_REV_1011
	.dcd.command.length	= cpu_to_be16(sizeof(struct imx_dcd_command) + sizeof(dcd_entry)),
#elif defined(CONFIG_TX53_REV_XX30)
	.dcd.command.length	= cpu_to_be16(0x21c),
#endif
	.dcd.command.param	= DCD_COMMAND_WRITE_PARAM,
};
