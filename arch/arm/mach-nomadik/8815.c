/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
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
#include <init.h>
#include <asm/hardware.h>
#include <mach/hardware.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>

#include "clock.h"

static struct clk st8815_clk_48 = {
       .rate = 48 * 1000 * 1000,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id = -1,
	.name = "mem",
	.map_base = 0x00000000,
	.platform_data = &ram_pdata,
};

void st8815_add_device_sdram(u32 size)
{
	sdram_dev.size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

static struct device_d uart0_serial_device = {
	.id = 0,
	.name = "uart-pl011",
	.map_base = NOMADIK_UART0_BASE,
	.size = 4096,
};

static struct device_d uart1_serial_device = {
	.id = 1,
	.name = "uart-pl011",
	.map_base = NOMADIK_UART1_BASE,
	.size = 4096,
};

void st8815_register_uart(unsigned id)
{
	switch (id) {
	case 0:
		nmdk_clk_create(&st8815_clk_48, dev_name(&uart0_serial_device));
		register_device(&uart0_serial_device);
		break;
	case 1:
		nmdk_clk_create(&st8815_clk_48, dev_name(&uart1_serial_device));
		register_device(&uart1_serial_device);
		break;
	}
}
