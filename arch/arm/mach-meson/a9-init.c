// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "meson-a9: " fmt

#include <memory.h>
#include <of.h>
#include <asm/memory.h>
#include <linux/minmax.h>
#include <linux/printk.h>
#include <mach/meson/a9.h>
#include <mach/meson/bootsource.h>

/* The boot firmware knows the true DRAM size, the device tree may not */
int meson_a9_mem_init(void)
{
	u64 size, low;
	int ret;

	size = meson_a9_dram_size();
	if (!size) {
		pr_warn("no DRAM size from the firmware\n");
		return 0;
	}

	low = min_t(u64, size, MESON_A9_DRAM_LOW_MAXSIZE);

	ret = arm_add_mem_device("ram0", 0, low);
	if (ret || size == low)
		return ret;

	return arm_add_mem_device("ram1", MESON_A9_DRAM_HIGH_BASE, size - low);
}

static bool meson_a9_reserved(resource_size_t start, resource_size_t end)
{
	struct memory_bank *bank;
	struct resource *rsv;

	for_each_memory_bank(bank)
		for_each_reserved_region(bank, rsv)
			if (rsv->start <= start && end <= rsv->end)
				return true;

	return false;
}

/* Reserve BL31 and BL32 for us and the OS unless the device tree does */
static void meson_a9_reserve_firmware(const char *name, resource_size_t start,
				      resource_size_t size)
{
	resource_size_t end = start + size - 1;
	struct resource *res;

	/* a firmware without BL32 reports a size of zero */
	if (!size || meson_a9_reserved(start, end))
		return;

	res = reserve_sdram_region(name, start, size);
	if (!res) {
		pr_warn("cannot reserve %s at %pa-%pa\n", name, &start, &end);
		return;
	}

	pr_debug("reserved %s at %pa-%pa\n", name, &start, &end);

	of_register_fixup(of_fixup_reserved_memory, res);
}

int meson_a9_reserve_init(void)
{
	u32 sizes;

	sizes = readl(IOMEM(MESON_A9_SEC_STATUS_REG15));

	meson_a9_reserve_firmware("bl31", readl(IOMEM(MESON_A9_SEC_STATUS_REG17)),
				  (resource_size_t)(sizes >> 16) * SZ_64K);
	meson_a9_reserve_firmware("bl32", readl(IOMEM(MESON_A9_SEC_STATUS_REG16)),
				  (resource_size_t)(sizes & 0xffff) * SZ_64K);

	return 0;
}

static void meson_a9_watchdog_stop(void)
{
	u32 ctrl;

	// TODO: make this configurable, so full barebox-runtime can be
	// watchdog-monitored as well?
	ctrl = readl(IOMEM(MESON_A9_WATCHDOG_CTRL));
	if (ctrl & MESON_A9_WATCHDOG_CTRL_EN)
		writel(ctrl & ~MESON_A9_WATCHDOG_CTRL_EN, IOMEM(MESON_A9_WATCHDOG_CTRL));
}

int meson_a9_init(void)
{
	meson_a9_watchdog_stop();

	meson_bootsource_set(meson_a9_boot_device());

	return 0;
}
