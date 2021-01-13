// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Pengutronix e.K.
 *
 */
#include <common.h>
#include <init.h>
#include <of.h>
#include <asm/system_info.h>
#include <asm/barebox-arm.h>

extern char __dtb_overlay_of_flash_start[];

static int replace_dtb(void) {
	struct device_node *overlay;
	void *fdt;
	struct device_node *root;

	fdt = barebox_arm_boot_dtb();
	if (fdt)
		pr_debug("using boarddata provided DTB\n");

	if (!fdt) {
		pr_debug("No DTB found\n");
		return 0;
	}

	root = of_unflatten_dtb(fdt);

	if (!of_device_is_compatible(root, "linux,dummy-virt")) {
		of_delete_node(root);
		return 0;
	}

	overlay = of_unflatten_dtb(__dtb_overlay_of_flash_start);
	of_overlay_apply_tree(root, overlay);
	barebox_register_of(root);

	return 0;
}

pure_initcall(replace_dtb);

static int virt_probe(struct device_d *dev)
{
	char *hostname = "virt";

	if (cpu_is_cortex_a7())
		hostname = "virt-a7";
	else if (cpu_is_cortex_a15())
		hostname = "virt-a15";

	barebox_set_model("ARM QEMU virt");
	barebox_set_hostname(hostname);

	return 0;
}

static const struct of_device_id virt_of_match[] = {
	{ .compatible = "linux,dummy-virt" },
	{ /* Sentinel */},
};

static struct driver_d virt_board_driver = {
	.name = "board-qemu-virt",
	.probe = virt_probe,
	.of_compatible = virt_of_match,
};

postcore_platform_driver(virt_board_driver);
