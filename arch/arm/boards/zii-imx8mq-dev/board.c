// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <bootsource.h>
#include <common.h>
#include <envfs.h>
#include <init.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <mach/bbu.h>
#include "../zii-common/pn-fixup.h"

#define LRU_FLAG_EGALAX		BIT(0)
#define LRU_FLAG_NO_DEB		BIT(1)

struct zii_imx8mq_dev_lru_fixup {
	struct zii_pn_fixup fixup;
	unsigned int flags;
};

static int zii_imx8mq_dev_init(void)
{
	if (!of_machine_is_compatible("zii,imx8mq-ultra"))
		return 0;

	if (of_machine_is_compatible("zii,imx8mq-ultra-zest"))
		barebox_set_hostname("zest");
	if (of_machine_is_compatible("zii,imx8mq-ultra-rmb3"))
		barebox_set_hostname("rmb3");

	imx8mq_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0",
						     BBU_HANDLER_FLAG_DEFAULT);
	imx8mq_bbu_internal_mmc_register_handler("SD", "/dev/mmc1", 0);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-emmc");
	else
		of_device_enable_path("/chosen/environment-sd");

	defaultenv_append_directory(defaultenv_zii_common);
	defaultenv_append_directory(defaultenv_imx8mq_zii_dev);

	return 0;
}
device_initcall(zii_imx8mq_dev_init);

static int zii_imx8mq_dev_fixup_egalax_ts(struct device_node *root, void *ctx)
{
	struct device_node *np, * aliases;

	/*
	 * The 27" unit has a EETI eGalax touchscreen instead of the
	 * Synaptics RMI4 found on other units.
	 */
	pr_info("Enabling eGalax touchscreen instead of RMI4\n");

	np = of_find_compatible_node(root, NULL, "syna,rmi4-i2c");
	if (!np)
		return -ENODEV;

	of_device_disable(np);

	np = of_find_compatible_node(root, NULL, "eeti,exc3000");
	if (!np)
		return -ENODEV;

	of_device_enable(np);

	aliases = of_find_node_by_path_from(root, "/aliases");
	if (!aliases)
		return -ENODEV;

	of_property_write_string(aliases, "touchscreen0", np->full_name);

	return 0;
}

static int zii_imx8mq_dev_fixup_deb_internal(void)
{
	struct device_node *np, *aliases;
	struct device_d *dev;

	/*
	 * In the internal DT remove the complete FEC hierarchy and move the
	 * i210 to be the eth0 interface to allow network boot to work without
	 * rewriting all the boot scripts.
	 */
	aliases = of_find_node_by_path("/aliases");
	if (!aliases)
		return -ENODEV;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx8mq-fec");
	if (!np)
		return -ENODEV;

	of_device_disable(np);

	of_property_write_string(aliases, "ethernet1", np->full_name);

	dev = of_find_device_by_node(np);
	if (!dev)
		return -ENODEV;

	unregister_device(dev);

	np = of_find_node_by_name(NULL, "i210@0");
	if (!np)
		return -ENODEV;

	of_property_write_string(aliases, "ethernet0", np->full_name);

	/* Refresh the internal aliases list from the patched DT */
	of_alias_scan();

	/*
	 * Disable switch watchdog to make rave_reset_switch a no-op
	 */
	np = of_find_compatible_node(NULL, NULL, "zii,rave-wdt");
	if (!np)
		return -ENODEV;

	of_device_disable(np);

	return 0;
}

static int zii_imx8mq_dev_fixup_deb(struct device_node *root, void *ctx)
{
	struct device_node *np, *aliases;
	struct property *pp;

	/*
	 * In the kernel DT remove all devices from the DEB, which isn't
	 * present on this system.
	 */
	np = of_find_compatible_node(root, NULL, "marvell,mv88e6085");
	if (!np)
		return -ENODEV;

	of_device_disable(np);

	np = of_find_compatible_node(root, NULL, "zii,rave-wdt");
	if (!np)
		return -ENODEV;

	of_device_disable(np);

	aliases = of_find_node_by_path_from(root, "/aliases");
	if (!aliases)
		return -ENODEV;

	pp = of_find_property(aliases, "ethernet-switch0", NULL);
	if (!pp)
		return -ENODEV;

	of_delete_property(pp);

	return 0;
}

static void zii_imx8mq_dev_lru_fixup(const struct zii_pn_fixup *context)
{
	const struct zii_imx8mq_dev_lru_fixup *fixup =
		container_of(context, struct zii_imx8mq_dev_lru_fixup, fixup);

	if (fixup->flags & LRU_FLAG_EGALAX)
		of_register_fixup(zii_imx8mq_dev_fixup_egalax_ts, NULL);

	if (fixup->flags & LRU_FLAG_NO_DEB) {
		zii_imx8mq_dev_fixup_deb_internal();
		of_register_fixup(zii_imx8mq_dev_fixup_deb, NULL);
	}
}

#define ZII_IMX8MQ_DEV_LRU_FIXUP(__pn, __flags)		\
	{						\
		{ __pn, zii_imx8mq_dev_lru_fixup },	\
		__flags					\
	}

static const struct zii_imx8mq_dev_lru_fixup zii_imx8mq_dev_lru_fixups[] = {
	ZII_IMX8MQ_DEV_LRU_FIXUP("00-5131-02", LRU_FLAG_EGALAX),
	ZII_IMX8MQ_DEV_LRU_FIXUP("00-5131-03", LRU_FLAG_EGALAX),
	ZII_IMX8MQ_DEV_LRU_FIXUP("00-5170-01", LRU_FLAG_NO_DEB),
};

/*
 * This initcall needs to be executed before coredevices, so we have a chance
 * to fix up the devices with the correct information.
 */
static int zii_imx8mq_dev_process_fixups(void)
{
	if (!of_machine_is_compatible("zii,imx8mq-ultra"))
		return 0;

	zii_process_lru_fixups(zii_imx8mq_dev_lru_fixups);

	return 0;
}
postmmu_initcall(zii_imx8mq_dev_process_fixups);
