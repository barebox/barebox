// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Pengutronix e.K.
 *
 */
#include <common.h>
#include <init.h>
#include <of.h>
#include <memory.h>
#include <compressed-dtb.h>
#include <deep-probe.h>
#include <security/policy.h>
#include "qemu-virt-flash.h"
#include "commandline.h"

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
extern char __dtbo_qemu_virt_flash_nonsecure_start[];

static const struct of_device_id virt_of_match[] = {
	{ .compatible = "linux,dummy-virt", .data = arm_virt_init },
	{ .compatible = "riscv-virtio" },
	{ /* Sentinel */},
};
BAREBOX_DEEP_PROBE_ENABLE(virt_of_match);

static bool is_qemu_virt;

/*
 * We don't have a dedicated qemu-virt device tree and instead rely
 * on what Qemu passes us. To be able to get fundamental changes
 * in very early, we forego having a board driver here and do this
 * directly in the initcall.
 */
static int virt_board_driver_init(void)
{
	struct device_node *root = of_get_root_node();
	struct device_node *flash;
	const struct of_device_id *id;
	void (*init)(void);

	id = of_match_node(virt_of_match, root);
	if (!id)
		return 0;

	is_qemu_virt = true;

	if (id->data) {
		init = id->data;
		init();
	}

	/*
	 * Catch both old Qemu versions that place /flash in /soc and
	 * configurations, where the first flash bank is secure-world only
	 */
	flash = of_find_node_by_path(PARTS_TARGET_PATH_STR);
	if (flash && of_device_is_available(flash)) {
		of_overlay_apply_dtbo(root, __dtbo_qemu_virt_flash_start);
	} else if (IS_ENABLED(CONFIG_ARM)) {
		flash = of_find_node_by_path("/flash@4000000");
		if (flash && of_device_is_available(flash))
			of_overlay_apply_dtbo(root, __dtbo_qemu_virt_flash_nonsecure_start);
	}


	/* fragment may have added aliases to the DT */
	of_alias_scan();

	/* of_probe() will happen later at of_populate_initcall */

	security_policy_add(qemu_virt_factory);
	security_policy_add(qemu_virt_lockdown);
	/*
	 * qemu_virt_devel & qemu_virt_tamper intentionally not added here,
	 * so the test suite can exercise CONFIG_SECURITY_POLICY_PATH.
	 */

	qemu_virt_parse_commandline(root);

	return 0;
}
postcore_initcall(virt_board_driver_init);

static __maybe_unused int virt_board_reserve_fdt(void)
{
	resource_size_t start = ~0, end = ~0;

	if (!is_qemu_virt)
		return 0;

	memory_bank_first_find_space(&start, &end);
	if (start != ~0) {
		void *fdt = (void *)start;

		if (!blob_is_fdt(fdt))
			return 0;

		request_sdram_region("qemu-virt-fdt", start,
				     fdt_totalsize(fdt),
				     MEMTYPE_BOOT_SERVICES_DATA,
				     MEMATTRS_RO);
	}

	return 0;
}
#ifdef CONFIG_ARM
postmem_initcall(virt_board_reserve_fdt);
#endif
