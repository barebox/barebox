// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 */
#include <common.h>
#include <envfs.h>
#include <init.h>
#include <of.h>
#include <deep-probe.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <asm/system.h>
#include <of_address.h>

#define EXYNOS8895_DECON0	0x12860000
#define EXYNOS990_DECON0	0x19050000
#define HW_SW_TRIG_CONTROL	0x70
#define TRIG_AUTO_MASK_EN	BIT(12)
#define SW_TRIG_EN		BIT(8)
#define HW_TRIG_EN		BIT(0)

static int exynos_postcore_init(void)
{
	void __iomem *trig_ctrl;

	/*
	 * ARM64 s-boot usually keeps framebuffer refreshing disabled after
	 * jumping to kernel. Set the required bit so we can have output. This
	 * should ideally be dropped from board files once we have a decon
	 * driver.
	 */
	if (of_machine_is_compatible("samsung,exynos8895"))
		trig_ctrl = IOMEM(EXYNOS8895_DECON0 + HW_SW_TRIG_CONTROL);
	else if (of_machine_is_compatible("samsung,exynos990"))
		trig_ctrl = IOMEM(EXYNOS990_DECON0 + HW_SW_TRIG_CONTROL);
	else
		return 0;

	writel(TRIG_AUTO_MASK_EN | SW_TRIG_EN | HW_TRIG_EN,
	       trig_ctrl);

	return 0;
}
coredevice_initcall(exynos_postcore_init);

static inline int exynos_init(struct device *dev)
{
	barebox_set_model(of_get_model());
	barebox_set_hostname("samsung-exynos");

	defaultenv_append_directory(defaultenv_exynos);

	return 0;
}

static const struct of_device_id exynos_of_match[] = {
	{ .compatible = "samsung,dreamlte" },
	{ .compatible = "samsung,x1s" },
	{ /* Sentinel */},
};

MODULE_DEVICE_TABLE(of, exynos_of_match);

static struct driver exynos_board_driver = {
	.name = "board-exynos",
	.probe = exynos_init,
	.of_compatible = exynos_of_match,
};
postcore_platform_driver(exynos_board_driver);
