/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
 * Copyright 2007-2011 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Based on U-Boot arch/powerpc/cpu/mpc85xx/fdt.c and
 * common/fdt_support.c - version git-2b26201.
 */
#include <common.h>
#include <init.h>
#include <errno.h>
#include <environment.h>
#include <asm/processor.h>
#include <mach/clock.h>
#include <of.h>

static void of_setup_crypto_node(void *blob)
{
	struct device_node *crypto_node;

	crypto_node = of_find_compatible_node(blob, NULL, "fsl,sec2.0");
	if (crypto_node == NULL)
		return;

	of_delete_node(crypto_node);
}

/* These properties specify whether the hardware supports the stashing
 * of buffer descriptors in L2 cache.
 */
static void fdt_add_enet_stashing(void *fdt)
{
	struct device_node *node;

	node = of_find_compatible_node(fdt, NULL, "gianfar");
	while (node) {
		of_set_property(node, "bd-stash", NULL, 0, 1);
		of_property_write_u32(node, "rx-stash-len", 96);
		of_property_write_u32(node, "rx-stash-idx", 0);
		node = of_find_compatible_node(node, NULL, "gianfar");
	}
}

static int fdt_stdout_setup(struct device_node *blob)
{
	struct device_node *node, *alias;
	char sername[9] = { 0 };
	const char *prop;
	struct console_device *cdev;
	int len;

	node = of_create_node(blob, "/chosen");
	if (node == NULL) {
		pr_err("%s: could not open /chosen node\n", __func__);
		goto error;
	}

	cdev = console_get_first_active();
	if (cdev)
		sprintf(sername, "serial%d", cdev->dev->id);
	else
		sprintf(sername, "serial%d", 0);

	alias = of_find_node_by_path_from(blob, "/aliases");
	if (!alias) {
		pr_err("%s: could not get aliases node.\n", __func__);
		goto error;
	}
	prop = of_get_property(alias, sername, &len);
	of_set_property(node, "linux,stdout-path", prop, len, 1);

	return 0;
error:
	return -ENODEV;
}

static int fdt_cpu_setup(struct device_node *blob)
{
	struct device_node *node;
	struct sys_info sysinfo;

	/* delete crypto node if not on an E-processor */
	if (!IS_E_PROCESSOR(get_svr()))
		of_setup_crypto_node(blob);

	fdt_add_enet_stashing(blob);
	fsl_get_sys_info(&sysinfo);

	node = of_find_node_by_type(blob, "cpu");
	while (node) {
		const uint32_t *reg;

		of_property_write_u32(node, "timebase-frequency",
				fsl_get_timebase_clock());
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqSystemBus);
		reg = of_get_property(node, "reg", NULL);
		of_property_write_u32(node, "clock-frequency",
				sysinfo.freqProcessor[*reg]);
		node = of_find_node_by_type(node, "cpu");
	}

	node = of_find_node_by_type(blob, "soc");
	if (node)
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqSystemBus);

	node = of_find_compatible_node(blob, NULL, "fsl,elbc");
	if (node)
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqLocalBus);

	node = of_find_compatible_node(blob, NULL, "ns16550");
	while (node) {
		of_property_write_u32(node, "clock-frequency",
				sysinfo.freqSystemBus);
		node = of_find_compatible_node(node, NULL, "ns16550");
	}

	fdt_stdout_setup(blob);

	return 0;
}

static int of_register_mpc85xx_fixup(void)
{
	return of_register_fixup(fdt_cpu_setup);
}
late_initcall(of_register_mpc85xx_fixup);
