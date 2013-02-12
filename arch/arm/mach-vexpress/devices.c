/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>

#include <linux/amba/bus.h>

#include <asm/memory.h>

#include <mach/devices.h>

void vexpress_a9_legacy_add_ddram(u32 ddr0_size, u32 ddr1_size)
{
	arm_add_mem_device("ram0", 0x60000000, ddr0_size);

	if (ddr1_size)
		arm_add_mem_device("ram1", 0x80000000, ddr1_size);
}


void vexpress_a9_legacy_register_uart(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = 0x10009000;
		break;
	case 1:
		start = 0x1000a000;
		break;
	case 2:
		start = 0x1000b000;
		break;
	case 3:
		start = 0x1000c000;
		break;
	default:
		return;
	}
	amba_apb_device_add(NULL, "uart-pl011", id, start, 4096, NULL, 0);
}

void vexpress_add_ddram(u32 size)
{
	arm_add_mem_device("ram1", 0x80000000, size);
}

void vexpress_register_uart(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = 0x1c090000;
		break;
	case 1:
		start = 0x1c0a0000;
		break;
	case 2:
		start = 0x1c0b0000;
		break;
	case 3:
		start = 0x1c0c0000;
		break;
	default:
		return;
	}
	amba_apb_device_add(NULL, "uart-pl011", id, start, 4096, NULL, 0);
}
