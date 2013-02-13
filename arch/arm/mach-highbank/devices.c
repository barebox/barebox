/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <sizes.h>

#include <linux/amba/bus.h>

#include <asm/memory.h>

#include <mach/devices.h>

void highbank_add_ddram(u32 size)
{
	arm_add_mem_device("ram1", 0x00000000, size);
	add_mem_device("nvram", 0xfff88000, SZ_32K, IORESOURCE_MEM_WRITEABLE);
}

void highbank_register_uart(void)
{
	amba_apb_device_add(NULL, "uart-pl011", DEVICE_ID_SINGLE, 0xfff36000, 4096, NULL, 0);
}

void highbank_register_ahci(void)
{
	add_generic_device("ahci", DEVICE_ID_SINGLE, NULL, 0xffe08000,
			0x10000, IORESOURCE_MEM, NULL);
}

void highbank_register_xgmac(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = 0xfff50000;
		break;
	case 1:
		start = 0xfff51000;
		break;
	default:
		return;
	}

	add_generic_device("hb-xgmac", id, NULL, start, 0x1000,
				IORESOURCE_MEM, NULL);
}

void highbank_register_gpio(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = 0xfff30000;
		break;
	case 1:
		start = 0xfff31000;
		break;
	case 2:
		start = 0xfff32000;
		break;
	case 3:
		start = 0xfff33000;
		break;
	default:
		return;
	}

	amba_apb_device_add(NULL, "pl061_gpio", id, start, 0x1000, NULL, 0);
}
