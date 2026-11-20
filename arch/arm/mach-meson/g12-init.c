// SPDX-License-Identifier: GPL-2.0-only

#include <asm/memory.h>
#include <mach/meson/bootsource.h>
#include <mach/meson/g12.h>

/* The boot firmware knows the true DRAM size, the device tree may not */
int meson_g12_mem_init(void)
{
	return arm_add_mem_device("ram0", 0, meson_g12_dram_size());
}

int meson_g12_init(void)
{
	meson_bootsource_set(meson_g12_boot_device());

	return 0;
}
