/*
 * (C) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (c) 2010 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
 *
 */
#include <common.h>
#include <mach/imx-flash-header.h>
#include <mach/imx25-regs.h>
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

struct imx_dcd_entry __dcd_entry_section dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0xb8001010, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x92100000, },
	{ .ptr_type = 1, .addr = 0x80000400, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xa2100000, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xb2100000, },
	{ .ptr_type = 1, .addr = 0x80000033, .val = 0xda, },
	{ .ptr_type = 1, .addr = 0x81000000, .val = 0xff, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x82216080, },
	{ .ptr_type = 4, .addr = 0xb8001004, .val = 0x00295729, },
	{ .ptr_type = 4, .addr = 0x53f80008, .val = 0x20034000, },
};

struct imx_flash_header __flash_header_section flash_header = {
	.app_code_jump_vector	= DEST_BASE + 0x1000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= DEST_BASE,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof(dcd_entry),
};

unsigned long __image_len_section barebox_len = DCD_BAREBOX_SIZE;
