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

#define FIRMWARE_DTB_BASE	0x1000

struct fdt_header *fdt = NULL;

static int highbank_mem_init(void)
{
	struct device_node *np;
	int ret;

	/* load by the firmware at 0x1000 */
	fdt = IOMEM(FIRMWARE_DTB_BASE);

	ret = of_unflatten_dtb(fdt);
	if (ret) {
		pr_warn("no dtb found at 0x1000 use default configuration\n");
		fdt = NULL;
		goto not_found;
	}

	np = of_find_node_by_path("/memory");
	if (!np) {
		pr_warn("no memory node use default configuration\n");
		goto not_found;
	}

	ret = of_add_memory(np, true);
	if (ret) {
		pr_warn("memory node: probe failed use default configuration\n");
		goto not_found;
	}

	pr_info("highbank: dtb probed memory size\n");

	return 0;
not_found:
	highbank_add_ddram(4089 << 20);
	return 0;
}
mem_initcall(highbank_mem_init);

static int highbank_devices_init(void)
{
	if (!fdt) {
		highbank_register_gpio(0);
		highbank_register_gpio(1);
		highbank_register_gpio(2);
		highbank_register_gpio(3);
		highbank_register_ahci();
		highbank_register_xgmac(0);
		highbank_register_xgmac(1);
	} else {
		devfs_add_partition("ram0", FIRMWARE_DTB_BASE, SZ_64K, DEVFS_PARTITION_FIXED, "dtb");
	}

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
