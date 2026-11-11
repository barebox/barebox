/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_MESON_GX_H
#define __MACH_MESON_GX_H

#include <io.h>
#include <linux/sizes.h>
#include <linux/minmax.h>

#define MESON_GX_AOBUS_BASE		0xc8100000
#define MESON_GX_UART_AO_BASE		(MESON_GX_AOBUS_BASE + 0x4c0)

/* Always-On secure general purpose register, written by the boot firmware */
#define MESON_GX_AO_SEC_GP_CFG0		(MESON_GX_AOBUS_BASE + (0x90 << 2))
#define MESON_GX_AO_MEM_SIZE_MASK	0xffff0000	/* DRAM size in MiB */
#define MESON_GX_AO_MEM_SIZE_SHIFT	16
#define MESON_GX_AO_BOOT_DEVICE_MASK	0xf

/* everything from here up is I/O, whatever the DRAM size */
#define MESON_GX_DRAM_MAXSIZE		0xc0000000

static inline unsigned long meson_gx_dram_size(void)
{
	u32 cfg0 = readl(IOMEM(MESON_GX_AO_SEC_GP_CFG0));
	unsigned long memsize;

	memsize = (cfg0 & MESON_GX_AO_MEM_SIZE_MASK) >> MESON_GX_AO_MEM_SIZE_SHIFT;

	return min_t(unsigned long, memsize * SZ_1M, MESON_GX_DRAM_MAXSIZE);
}

static inline unsigned int meson_gx_boot_device(void)
{
	return readl(IOMEM(MESON_GX_AO_SEC_GP_CFG0)) & MESON_GX_AO_BOOT_DEVICE_MASK;
}

int meson_gx_mem_init(void);
int meson_gx_init(void);

#endif
