/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/system_info.h>
#include <generated/mach-types.h>
#include <mach/devices.h>
#include <mach/hardware.h>
#include <mach/sysregs.h>
#include <environment.h>
#include <partition.h>
#include <linux/sizes.h>
#include <io.h>
#include <of.h>
#include <envfs.h>

#define FIRMWARE_DTB_BASE	0x1000

#define HB_OPP_VERSION		0

struct fdt_header *fdt = NULL;

static int hb_fixup(struct device_node *root, void *unused)
{
	struct device_node *node;
	u32 reg = readl(sregs_base + HB_SREG_A9_PWRDOM_DATA);
	u32 *opp_table = (u32 *)HB_SYSRAM_OPP_TABLE_BASE;
	u32 dtb_table[2*10];
	u32 i;
	u32 num_opps;
	__be32 latency;

	if (!(reg & HB_PWRDOM_STAT_SATA)) {
		for_each_compatible_node_from(node, root, NULL, "calxeda,hb-ahci")
			of_set_property(node, "status", "disabled",
					sizeof("disabled"), 1);
	}

	if (!(reg & HB_PWRDOM_STAT_EMMC)) {
		for_each_compatible_node_from(node, root, NULL, "calxeda,hb-sdhci")
			of_set_property(node, "status", "disabled",
					sizeof("disabled"), 1);
	}

	if ((opp_table[0] >> 16) != HB_OPP_VERSION)
		return 0;

	node = of_find_node_by_path("/cpus/cpu@0");
	if (!node)
		return 0;

	num_opps = opp_table[0] & 0xff;

	for (i = 0; i < num_opps; i++) {
		dtb_table[2 * i] = cpu_to_be32(opp_table[3 + 3 * i]);
		dtb_table[2 * i + 1] = cpu_to_be32(opp_table[2 + 3 * i]);
	}

	latency = cpu_to_be32(opp_table[1]);

	of_set_property(node, "transition-latency", &latency, 4, 1);
	of_set_property(node, "operating-points", dtb_table, 8 * num_opps, 1);

	return 0;
}

static int highbank_mem_init(void)
{
	struct device_node *root, *np;
	int ret;

	/* load by the firmware at 0x1000 */
	fdt = IOMEM(FIRMWARE_DTB_BASE);

	root = of_unflatten_dtb(fdt);
	if (IS_ERR(root)) {
		pr_warn("no dtb found at 0x1000 use default configuration\n");
		fdt = NULL;
		goto not_found;
	}

	of_set_root_node(root);

	np = of_find_node_by_path("/memory");
	if (!np) {
		pr_warn("no memory node use default configuration\n");
		goto not_found;
	}

	ret = of_add_memory(np, true);
	if (ret) {
		pr_warn("memory node: probe failed use default configuration\n");
		goto not_found;
	}

	pr_info("highbank: dtb probed memory size\n");

	return 0;
not_found:
	highbank_add_ddram(4089 << 20);
	return 0;
}
mem_initcall(highbank_mem_init);

static int highbank_devices_init(void)
{
	of_register_fixup(hb_fixup, NULL);
	if (!fdt) {
		highbank_register_gpio(0);
		highbank_register_gpio(1);
		highbank_register_gpio(2);
		highbank_register_gpio(3);
		highbank_register_ahci();
		highbank_register_xgmac(0);
		highbank_register_xgmac(1);
	} else {
		fdt = of_get_fixed_tree(NULL);
		add_mem_device("dtb", (unsigned long)fdt, be32_to_cpu(fdt->totalsize),
		       IORESOURCE_MEM_WRITEABLE);
		devfs_add_partition("ram0", FIRMWARE_DTB_BASE, SZ_64K, DEVFS_PARTITION_FIXED, "firmware-dtb");
	}

	devfs_add_partition("nvram", 0x00000, SZ_16K, DEVFS_PARTITION_FIXED, "env0");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_highbank);

	return 0;
}
device_initcall(highbank_devices_init);

static int highbank_console_init(void)
{
	barebox_set_model("Calxeda Highbank");
	barebox_set_hostname("highbank");

	highbank_register_uart();

	return 0;
}
console_initcall(highbank_console_init);
