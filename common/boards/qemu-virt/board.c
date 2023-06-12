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

extern char __dtbo_qemu_virt_flash_start[];
extern char __dtb_fitimage_pubkey_start[];

static const struct of_device_id virt_of_match[] = {
	{ .compatible = "linux,dummy-virt", .data = arm_virt_init },
	{ .compatible = "riscv-virtio" },
	{ /* Sentinel */},
};
BAREBOX_DEEP_PROBE_ENABLE(virt_of_match);

/*
 * We don't have a dedicated qemu-virt device tree and instead rely
 * on what Qemu passes us. To be able to get fundamental changes
 * in very early, we forego having a board driver here and do this
 * directly in the initcall.
 */
static int virt_board_driver_init(void)
{
	struct device_node *root = of_get_root_node();
	struct device_node *overlay, *pubkey;
	const struct of_device_id *id;
	void (*init)(void);

	id = of_match_node(virt_of_match, root);
	if (!id)
		return 0;

	if (id->data) {
		init = id->data;
		init();
	}

	overlay = of_unflatten_dtb(__dtbo_qemu_virt_flash_start, INT_MAX);
	of_overlay_apply_tree(root, overlay);

	pubkey = of_unflatten_dtb(__dtb_fitimage_pubkey_start, INT_MAX);
	of_merge_nodes(root, pubkey);

	/* of_probe() will happen later at of_populate_initcall */

	return 0;
}
postcore_initcall(virt_board_driver_init);
