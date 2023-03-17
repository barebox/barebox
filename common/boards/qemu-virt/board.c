// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Pengutronix e.K.
 *
 */
#include <common.h>
#include <init.h>
#include <of.h>
#include <deep-probe.h>

#ifdef CONFIG_64BIT
#define MACHINE "virt64"
#else
#define MACHINE "virt"
#endif

#ifdef CONFIG_ARM
#include <asm/system_info.h>

static inline void arm_virt_init(void)
{
	const char *hostname = MACHINE;

	if (cpu_is_cortex_a7())
		hostname = "virt-a7";
	else if (cpu_is_cortex_a15())
		hostname = "virt-a15";

	barebox_set_model("ARM QEMU " MACHINE);
	barebox_set_hostname(hostname);
}

#else
static inline void arm_virt_init(void) {}
#endif

extern char __dtb_overlay_of_flash_start[];
extern char __dtb_fitimage_pubkey_start[];

static int virt_probe(struct device *dev)
{
	struct device_node *overlay, *pubkey;
	void (*init)(void);

	init = device_get_match_data(dev);
	if (init)
		init();

	overlay = of_unflatten_dtb(__dtb_overlay_of_flash_start, INT_MAX);
	of_overlay_apply_tree(dev->of_node, overlay);

	pubkey = of_unflatten_dtb(__dtb_fitimage_pubkey_start, INT_MAX);
	of_merge_nodes(dev->of_node, pubkey);

	/* of_probe() will happen later at of_populate_initcall */

	return 0;
}

static const struct of_device_id virt_of_match[] = {
	{ .compatible = "linux,dummy-virt", .data = arm_virt_init },
	{ .compatible = "riscv-virtio" },
	{ /* Sentinel */},
};
BAREBOX_DEEP_PROBE_ENABLE(virt_of_match);

static struct driver virt_board_driver = {
	.name = "board-qemu-virt",
	.probe = virt_probe,
	.of_compatible = virt_of_match,
};

static int virt_board_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&virt_board_driver);
	if (ret)
		return ret;

	return of_devices_ensure_probed_by_dev_id(virt_of_match);
}
postcore_initcall(virt_board_driver_init);
