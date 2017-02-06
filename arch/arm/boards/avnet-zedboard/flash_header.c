/*
 * Copyright (C) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
#include <mach/zynq-flash-header.h>
#include <mach/zynq7000-regs.h>
#include <asm/barebox-arm-head.h>

#define REG(a, v) { .addr = cpu_to_le32(a), .val = cpu_to_le32(v), }

struct zynq_reg_entry __ps7reg_entry_section reg_entry[] = {
	REG(ZYNQ_SLCR_UNLOCK, 0x0000DF0D),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_CLK_621_TRUE, 0x00000001),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_APER_CLK_CTRL, 0x01FC044D),

	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL, 0x00028008),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CFG, 0x000FA220),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL, 0x00028010),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL, 0x00028011),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL, 0x00028010),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL, 0x00028000),

	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL, 0x0001E008),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CFG, 0x001452C0),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL, 0x0001E010),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL, 0x0001E011),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL, 0x0001E010),
	REG(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL, 0x0001E000),

	REG(0xf8000150, 0x00000a03),

	/* stop */
	REG(0xFFFFFFFF, 0x00000000),
};

struct zynq_flash_header __flash_header_section flash_header = {
	.width_det		= WIDTH_DETECTION_MAGIC,
	.image_id		= IMAGE_IDENTIFICATION,
	.enc_stat		= 0x0,
	.user			= 0x0,
	.flash_offset		= 0x8c0,
	.length			= (unsigned int)&_barebox_image_size,
	.res0			= 0x0,
	.start_of_exec		= 0x0,
	.total_len		= (unsigned int)&_barebox_image_size,
	.res1			= 0x1,
	.checksum		= 0x0,
	.res2			= 0x0,
};
