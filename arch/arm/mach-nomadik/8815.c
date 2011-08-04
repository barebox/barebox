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

void st8815_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", 0x00000000, size);
}

void st8815_register_uart(unsigned id)
{
	resource_size_t start;
	struct device_d *dev;

	switch (id) {
	case 0:
		start = NOMADIK_UART1_BASE;
		break;
	case 1:
		start = NOMADIK_UART1_BASE;
		break;
	}
	dev = add_generic_device("uart-pl011", id, NULL, start, 4096,
			   IORESOURCE_MEM, NULL);
	nmdk_clk_create(&st8815_clk_48, dev_name(dev));
}
