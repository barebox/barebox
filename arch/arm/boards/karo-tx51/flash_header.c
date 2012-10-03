/**
 * Copyright (C) 2012 Christian Kapeller, <christian.kapeller@cmotion.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx-flash-header.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

struct imx_dcd_entry __dcd_entry_section dcd_entry[] = {
    { .ptr_type = 4, .addr = 0x83fd9000, .val = 0x80000000, },
    { .ptr_type = 4, .addr = 0x83fd9014, .val = 0x04008008, },
    { .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008010, },
    { .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008010, },
    { .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00338018, },
    { .ptr_type = 4, .addr = 0x83fd9000, .val = 0xb2220000, },
    { .ptr_type = 4, .addr = 0x83fd9004, .val = 0xb08564a9, },
    { .ptr_type = 4, .addr = 0x83fd9034, .val = 0x20020000, },
    { .ptr_type = 4, .addr = 0x83fd9010, .val = 0x000a0080, },
    { .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00000000, },
};

#define APP_DEST	0x90000000

struct imx_flash_header __flash_header_section flash_header = {
	.app_code_jump_vector	= APP_DEST + 0x1000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= APP_DEST,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof (dcd_entry),
};

unsigned long __image_len_section barebox_len = DCD_BAREBOX_SIZE;
