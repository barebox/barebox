// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "meson-gx: " fmt

#include <linux/printk.h>
#include <asm/memory.h>
#include <mach/meson/bootsource.h>
#include <mach/meson/gx.h>

/* The boot firmware knows the true DRAM size, the device tree may not */
int meson_gx_mem_init(void)
{
	unsigned long size;

	size = meson_gx_dram_size();
	if (!size) {
		pr_warn("no DRAM size from the firmware\n");
		return 0;
	}

	return arm_add_mem_device("ram0", 0, size);
}

int meson_gx_init(void)
{
	meson_bootsource_set(meson_gx_boot_device());

	return 0;
}
