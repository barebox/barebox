/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/system_info.h>
#include <generated/mach-types.h>
#include <mach/devices.h>
#include <environment.h>
#include <partition.h>
#include <sizes.h>
#include <io.h>

static int highbank_mem_init(void)
{
	highbank_add_ddram(4089 << 20);

	return 0;
}
mem_initcall(highbank_mem_init);

static int highbank_devices_init(void)
{
	highbank_register_gpio(0);
	highbank_register_gpio(1);
	highbank_register_gpio(2);
	highbank_register_gpio(3);
	highbank_register_ahci();
	highbank_register_xgmac(0);
	highbank_register_xgmac(1);

	armlinux_set_bootparams((void *)(0x00000100));

	devfs_add_partition("nvram", 0x00000, SZ_16K, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
device_initcall(highbank_devices_init);

static int highbank_console_init(void)
{
	highbank_register_uart();

	return 0;
}
console_initcall(highbank_console_init);
