/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_MESON_A9_H
#define __MACH_MESON_A9_H

#include <io.h>
#include <linux/bits.h>
#include <linux/sizes.h>

#define MESON_A9_AOBUS_BASE		0xffa00000
#define MESON_A9_UART_B_BASE		(MESON_A9_AOBUS_BASE + 0x1e000)

/* Secure status registers, written by the boot firmware */
#define MESON_A9_AO_SYSCTRL_BASE	0xffa80000
#define MESON_A9_SEC_STATUS_REG2	(MESON_A9_AO_SYSCTRL_BASE + (0x3d << 2))
#define MESON_A9_BOOT_DEVICE_MASK	0xf0	/* MESON_BOOT_DEVICE_* */
#define MESON_A9_BOOT_DEVICE_SHIFT	4
#define MESON_A9_SEC_STATUS_REG4	(MESON_A9_AO_SYSCTRL_BASE + (0x3f << 2))
#define MESON_A9_MEM_SIZE_MASK		0xfff00000	/* DRAM size in 16 MiB */
#define MESON_A9_MEM_SIZE_SHIFT		20

/*
 * BL31 and BL32 in DRAM: REG15 has their sizes in 64 KiB, BL31's in the
 * upper half, REG17 BL31's start and REG16 BL32's
 */
#define MESON_A9_SYSCTRL_BASE		0xfe010000
#define MESON_A9_SEC_STATUS_REG15	(MESON_A9_SYSCTRL_BASE + (0xcf << 2))
#define MESON_A9_SEC_STATUS_REG16	(MESON_A9_SYSCTRL_BASE + (0xd0 << 2))
#define MESON_A9_SEC_STATUS_REG17	(MESON_A9_SYSCTRL_BASE + (0xd1 << 2))

#define MESON_A9_WATCHDOG_CTRL		0xfe002100
#define MESON_A9_WATCHDOG_CTRL_EN	BIT(18)

/* DRAM beyond the 32-bit I/O space continues above 4 GiB */
#define MESON_A9_DRAM_LOW_MAXSIZE	0xe0000000
#define MESON_A9_DRAM_HIGH_BASE		SZ_4G

static inline u64 meson_a9_dram_size(void)
{
	u32 reg4 = readl(IOMEM(MESON_A9_SEC_STATUS_REG4));

	return (u64)((reg4 & MESON_A9_MEM_SIZE_MASK) >> MESON_A9_MEM_SIZE_SHIFT) * SZ_16M;
}

static inline unsigned int meson_a9_boot_device(void)
{
	u32 reg2 = readl(IOMEM(MESON_A9_SEC_STATUS_REG2));

	return (reg2 & MESON_A9_BOOT_DEVICE_MASK) >> MESON_A9_BOOT_DEVICE_SHIFT;
}

int meson_a9_mem_init(void);
int meson_a9_reserve_init(void);
int meson_a9_init(void);

#endif
