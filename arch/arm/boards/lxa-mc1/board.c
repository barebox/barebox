// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/bbu.h>
#include <bootsource.h>
#include <of.h>

static int of_fixup_regulator_supply_disable(struct device_node *root, void *path)
{
	struct device_node *node;
	struct property *prop;

	node = of_find_node_by_path_from(root, path);
	if (!node) {
		pr_warn("fixup for %s failed: not found\n", (const char *)path);
		return -ENOENT;
	}

	if (!of_property_read_bool(node, "regulator-always-on"))
		return 0;

	prop = of_find_property(node, "vin-supply", NULL);
	if (prop)
		of_delete_property(prop);

	return 0;
}

static int mc1_device_init(void)
{
	int flags;
	if (!of_machine_is_compatible("lxa,stm32mp157c-mc1"))
		return 0;

	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("sd", "/dev/mmc0.ssbl", flags);

	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	stm32mp_bbu_mmc_register_handler("emmc", "/dev/mmc1.ssbl", flags);


	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	barebox_set_hostname("lxa-mc1");

	/* The regulator is powered by the PMIC, but is always on as far as
	 * software is concerned. Break the reference to the PMIC, so the OS
	 * doesn't need to defer SDMMC/Ethernet peripherals till after the PMIC
	 * is up.
	 */
	return of_register_fixup(of_fixup_regulator_supply_disable, "/regulator_3v3");
}
device_initcall(mc1_device_init);
