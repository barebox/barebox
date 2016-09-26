/*
 * Copyright (C) 2016 Raphaël Poggi <poggi.raph@gmail.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/system_info.h>
#include <mach/devices.h>
#include <environment.h>
#include <linux/sizes.h>
#include <io.h>
#include <envfs.h>
#include <globalvar.h>
#include <asm/mmu.h>

static int virt_mem_init(void)
{
	virt_add_ddram(SZ_2G);

	return 0;
}
mem_initcall(virt_mem_init);

static int virt_env_init(void)
{
	add_cfi_flash_device(0, 0x00000000, SZ_128M, 0);

	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_qemu_virt64);

	return 0;
}
device_initcall(virt_env_init);

static int virt_console_init(void)
{
	virt_register_uart(0);

	return 0;
}
console_initcall(virt_console_init);

static int virt_core_init(void)
{
	char *hostname = "virt64";

	if (cpu_is_cortex_a53())
		hostname = "virt64-a53";
	else if (cpu_is_cortex_a57())
		hostname = "virt64-a57";

	barebox_set_model("ARM QEMU virt64");
	barebox_set_hostname(hostname);

	return 0;
}
postcore_initcall(virt_core_init);

#ifdef CONFIG_MMU
static int virt_mmu_enable(void)
{
	/* Mapping all periph and flash range */
	arch_remap_range((void *)0x00000000, 0x40000000, DEV_MEM);

	mmu_enable();

	return 0;
}
postmmu_initcall(virt_mmu_enable);
#endif
