/*
 * Copyright (C) 2016 Raphaël Poggi <poggi.raph@gmail.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <linux/amba/bus.h>
#include <asm/memory.h>
#include <mach/devices.h>
#include <linux/ioport.h>

void virt_add_ddram(u32 size)
{
	arm_add_mem_device("ram0", 0x40000000, size);
}

void virt_register_uart(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = 0x09000000;
		break;
	default:
		return;
	}
	amba_apb_device_add(NULL, "uart-pl011", id, start, 4096, NULL, 0);
}
