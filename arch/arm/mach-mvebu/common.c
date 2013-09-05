/*
 * Copyright (C) 2013
 *  Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *  Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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
#include <io.h>
#include <sizes.h>
#include <asm/barebox-arm.h>
#include <mach/common.h>

/*
 * All MVEBU SoCs start with internal registers at 0xd0000000.
 * To get more contiguous address space and as Linux expects them
 * there, we remap them early to 0xf1000000.
 *
 * There is no way to determine internal registers base address
 * safely later on, as the remap register itself is within the
 * internal registers.
 */
#define MVEBU_BOOTUP_INT_REG_BASE	0xd0000000
#define MVEBU_BRIDGE_REG_BASE		0x20000
#define DEVICE_INTERNAL_BASE_ADDR	(MVEBU_BRIDGE_REG_BASE + 0x80)

static void mvebu_remap_registers(void)
{
	writel(MVEBU_REMAP_INT_REG_BASE,
	       IOMEM(MVEBU_BOOTUP_INT_REG_BASE) + DEVICE_INTERNAL_BASE_ADDR);
}

/*
 * Determining the actual memory size is highly SoC dependent,
 * but for all SoCs RAM starts at 0x00000000. Therefore, we start
 * with a minimal memory setup of 64M and probe correct memory size
 * later.
 */
#define MVEBU_BOOTUP_MEMORY_BASE	0x00000000
#define MVEBU_BOOTUP_MEMORY_SIZE	SZ_64M

void __naked __noreturn mvebu_barebox_entry(uint32_t boarddata)
{
	mvebu_remap_registers();
	barebox_arm_entry(MVEBU_BOOTUP_MEMORY_BASE,
			  MVEBU_BOOTUP_MEMORY_SIZE, boarddata);
}
