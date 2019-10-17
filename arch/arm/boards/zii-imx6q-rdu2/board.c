/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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
 */

#include <common.h>
#include <envfs.h>
#include <fs.h>
#include <gpio.h>
#include <i2c/i2c.h>
#include <init.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <net.h>
#include <linux/nvmem-consumer.h>
#include "../zii-common/pn-fixup.h"

enum rdu2_lcd_interface_type {
	IT_SINGLE_LVDS,
	IT_DUAL_LVDS,
	IT_EDP
};

enum rdu2_lvds_busformat {
	BF_NONE,
	BF_JEIDA,
	BF_SPWG
};

#define RDU2_LRU_FLAG_EGALAX	BIT(0)
#define RDU2_LRU_FLAG_NO_FEC	BIT(1)

struct rdu2_lru_fixup {
	struct zii_pn_fixup fixup;
	unsigned int flags;
	enum rdu2_lcd_interface_type type;
	enum rdu2_lvds_busformat bus_format;
	const char *compatible;
};

#define RDU2_DAC1_RESET	IMX_GPIO_NR(1, 0)
#define RDU2_DAC2_RESET	IMX_GPIO_NR(1, 2)
#define RDU2_RST_TOUCH	IMX_GPIO_NR(1, 7)
#define RDU2_NFC_RESET	IMX_GPIO_NR(1, 17)
#define RDU2_HPA1_SDn	IMX_GPIO_NR(1, 4)
#define RDU2_HPA2_SDn	IMX_GPIO_NR(1, 5)

static const struct gpio rdu2_reset_gpios[] = {
	{
		.gpio = RDU2_DAC1_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac1-reset",
	},
	{
		.gpio = RDU2_DAC2_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac2-reset",
	},
	{
		.gpio = RDU2_RST_TOUCH,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "rst-touch#",
	},
	{
		.gpio = RDU2_NFC_RESET,
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "nfc-reset",
	},
	{
		.gpio = RDU2_HPA1_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa1-sd-n",
	},
	{
		.gpio = RDU2_HPA2_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa2n-sd-n",
	},
};

static int rdu2_reset_audio_touchscreen_nfc(void)
{
	int ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	ret = gpio_request_array(rdu2_reset_gpios,
				 ARRAY_SIZE(rdu2_reset_gpios));
	if (ret) {
		pr_err("Failed to request RDU2 reset gpios: %s\n",
		       strerror(-ret));
		return ret;
	}

	mdelay(100);

	gpio_direction_output(RDU2_DAC1_RESET, 1);
	gpio_direction_output(RDU2_DAC2_RESET, 1);
	gpio_direction_output(RDU2_RST_TOUCH,  1);
	gpio_direction_output(RDU2_NFC_RESET,  0);
	gpio_direction_output(RDU2_HPA1_SDn,   1);
	gpio_direction_output(RDU2_HPA2_SDn,   1);

	mdelay(100);

	return 0;
}
/*
 * When this function is called "hog" pingroup in device tree needs to
 * be already initialized
 */
late_initcall(rdu2_reset_audio_touchscreen_nfc);

static int rdu2_devices_init(void)
{
	struct i2c_client client;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	client.adapter = i2c_get_adapter(1);
	if (client.adapter) {
		u8 reg;

		/*
		 * Reset PMIC SW1AB and SW1C rails to 1.375V. If an event
		 * caused only the i.MX6 SoC reset, the PMIC might still be
		 * stuck on the low voltage for the slow operating point.
		 */
		client.addr = 0x08; /* PMIC i2c address */
		reg = 0x2b; /* 1.375V, valid for both rails */
		i2c_write_reg(&client, 0x20, &reg, 1);
		i2c_write_reg(&client, 0x2e, &reg, 1);
	}

	barebox_set_hostname("rdu2");

	imx6_bbu_internal_spi_i2c_register_handler("SPI", "/dev/m25p0.barebox",
						   BBU_HANDLER_FLAG_DEFAULT);

	imx6_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc3", 0);

	defaultenv_append_directory(defaultenv_rdu2);

	return 0;
}
device_initcall(rdu2_devices_init);

static int rdu2_fixup_egalax_ts(struct device_node *root, void *context)
{
	struct device_node *np;

	/*
	 * The 32" unit has a EETI eGalax touchscreen instead of the
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
	of_property_write_u32(np->parent, "clock-frequency", 200000);


	return 0;
}

static int rdu2_fixup_dsa(struct device_node *root, void *context)
{
	struct device_node *switch_np, *np;
	phandle i210_handle;

	/*
	 * The 12.1" unit has no FEC connection, so we need to rewrite
	 * the i210 port into the CPU port and delete the FEC port,
	 * which is part of the common setup.
	 */
	pr_info("Rewriting i210 switch port into CPU port\n");

	switch_np = of_find_compatible_node(root, NULL, "marvell,mv88e6085");
	if (!switch_np)
		return -ENODEV;

	np = of_find_node_by_name(switch_np, "port@2");
	if (!np)
		return -ENODEV;

	of_delete_node(np);

	np = of_find_node_by_name(root, "i210@0");
	if (!np)
		return -ENODEV;

	i210_handle = of_node_create_phandle(np);

	np = of_find_node_by_name(switch_np, "port@0");
	if (!np)
		return -ENODEV;

	of_property_write_u32(np, "ethernet", i210_handle);
	of_property_write_string(np, "label", "cpu");

	return 0;
}

static int rdu2_fixup_edp(struct device_node *root)
{
	const bool kernel_fixup = root != NULL;
	struct device_node *np;

	if (kernel_fixup) {
		/*
		 * Kernel DT fixup needs this additional step
		 */
		pr_info("Found eDP display, enabling parallel output "
			"and eDP bridge.\n");
		np = of_find_compatible_node(root, NULL,
					     "fsl,imx-parallel-display");
		if (!np)
			return -ENODEV;

		of_device_enable(np);
	}

	np = of_find_compatible_node(root, NULL, "toshiba,tc358767");
	if (!np)
		return -ENODEV;

	of_device_enable(np);

	return 0;
}

static int rdu2_fixup_lvds(struct device_node *root,
			   const struct rdu2_lru_fixup *fixup)
{
	const bool kernel_fixup = root != NULL;
	struct device_node *np;

	/*
	 * LVDS panels need the correct compatible
	 */
	pr_info("Found LVDS display, enabling %s channel LDB and "
		"panel with compatible \"%s\".\n",
		fixup->type == IT_DUAL_LVDS ? "dual" : "single",
		fixup->compatible);
	/*
	 * LVDS panels need the correct timings
	 */
	np = of_find_node_by_name(root, "panel");
	if (!np)
		return -ENODEV;

	if (kernel_fixup) {
		of_device_enable(np);
		of_property_write_string(np, "compatible", fixup->compatible);
	} else {
		struct device_node *child, *tmp;

		of_device_enable_and_register(np);
		/*
		 * Delete all mode entries, which aren't suited for the
		 * current display
		 */
		np = of_find_node_by_name(np, "display-timings");
		if (!np)
			return -ENODEV;

		for_each_child_of_node_safe(np, tmp, child) {
			if (!of_device_is_compatible(child,
						     fixup->compatible))
				of_delete_node(child);
		}
	}
	/*
	 * enable LDB channel 0 and set correct interface mode
	 */
	np = of_find_compatible_node(root, NULL, "fsl,imx6q-ldb");
	if (!np)
		return -ENODEV;

	if (kernel_fixup)
		of_device_enable(np);
	else
		of_device_enable_and_register(np);

	if (fixup->type == IT_DUAL_LVDS)
		of_set_property(np, "fsl,dual-channel", NULL, 0, 1);

	np = of_find_node_by_name(np, "lvds-channel@0");
	if (!np)
		return -ENODEV;

	of_device_enable(np);

	if (!kernel_fixup) {
		of_property_write_string(np, "fsl,data-mapping",
					 fixup->bus_format == BF_SPWG ?
					 "spwg" : "jeida");
	}

	return 0;
}

static int rdu2_fixup_display(struct device_node *root, void *context)
{
	const struct rdu2_lru_fixup *fixup = context;
	/*
	 * If the panel is eDP, just enable the parallel output and
	 * eDP bridge
	 */
	if (fixup->type == IT_EDP)
		return rdu2_fixup_edp(root);

	return rdu2_fixup_lvds(root, context);
}

static void rdu2_lru_fixup(const struct zii_pn_fixup *context)
{
	const struct rdu2_lru_fixup *fixup =
		container_of(context, struct rdu2_lru_fixup, fixup);

	WARN_ON(rdu2_fixup_display(NULL, (void *)context));
	of_register_fixup(rdu2_fixup_display, (void *)context);

	if (fixup->flags & RDU2_LRU_FLAG_EGALAX)
		of_register_fixup(rdu2_fixup_egalax_ts, NULL);

	if (fixup->flags & RDU2_LRU_FLAG_NO_FEC)
		of_register_fixup(rdu2_fixup_dsa, NULL);
}

#define RDU2_LRU_FIXUP(__pn, __flags, __panel)		\
	{						\
		{ __pn, rdu2_lru_fixup },		\
		__flags,				\
		__panel					\
	}

#define RDU2_PANEL_10P1	IT_SINGLE_LVDS, BF_SPWG,  "innolux,g121i1-l01"
#define RDU2_PANEL_11P6	IT_EDP,         BF_NONE,  NULL
#define RDU2_PANEL_12P1	IT_SINGLE_LVDS, BF_SPWG,  "nec,nl12880bc20-05"
#define RDU2_PANEL_13P3	IT_DUAL_LVDS,   BF_JEIDA, "auo,g133han01"
#define RDU2_PANEL_15P6	IT_DUAL_LVDS,   BF_SPWG,  "nlt,nl192108ac18-02d"
#define RDU2_PANEL_18P5	IT_DUAL_LVDS,   BF_SPWG,  "auo,g185han01"
#define RDU2_PANEL_32P0	IT_DUAL_LVDS,   BF_SPWG,  "auo,p320hvn03"

static const struct rdu2_lru_fixup rdu2_lru_fixups[] = {
	RDU2_LRU_FIXUP("00-5122-01", RDU2_LRU_FLAG_NO_FEC, RDU2_PANEL_12P1),
	RDU2_LRU_FIXUP("00-5122-02", RDU2_LRU_FLAG_NO_FEC, RDU2_PANEL_12P1),
	RDU2_LRU_FIXUP("00-5120-01", 0, RDU2_PANEL_10P1),
	RDU2_LRU_FIXUP("00-5120-02", 0, RDU2_PANEL_10P1),
	RDU2_LRU_FIXUP("00-5120-51", 0, RDU2_PANEL_10P1),
	RDU2_LRU_FIXUP("00-5120-52", 0, RDU2_PANEL_10P1),
	RDU2_LRU_FIXUP("00-5123-01", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5123-02", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5123-03", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5123-51", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5123-52", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5123-53", 0, RDU2_PANEL_11P6),
	RDU2_LRU_FIXUP("00-5124-01", 0, RDU2_PANEL_13P3),
	RDU2_LRU_FIXUP("00-5124-02", 0, RDU2_PANEL_13P3),
	RDU2_LRU_FIXUP("00-5124-03", 0, RDU2_PANEL_13P3),
	RDU2_LRU_FIXUP("00-5124-53", 0, RDU2_PANEL_13P3),
	RDU2_LRU_FIXUP("00-5127-01", 0, RDU2_PANEL_15P6),
	RDU2_LRU_FIXUP("00-5127-02", 0, RDU2_PANEL_15P6),
	RDU2_LRU_FIXUP("00-5127-03", 0, RDU2_PANEL_15P6),
	RDU2_LRU_FIXUP("00-5127-53", 0, RDU2_PANEL_15P6),
	RDU2_LRU_FIXUP("00-5125-01", 0, RDU2_PANEL_18P5),
	RDU2_LRU_FIXUP("00-5125-02", 0, RDU2_PANEL_18P5),
	RDU2_LRU_FIXUP("00-5125-03", 0, RDU2_PANEL_18P5),
	RDU2_LRU_FIXUP("00-5125-53", 0, RDU2_PANEL_18P5),
	RDU2_LRU_FIXUP("00-5132-01", RDU2_LRU_FLAG_EGALAX, RDU2_PANEL_32P0),
	RDU2_LRU_FIXUP("00-5132-02", RDU2_LRU_FLAG_EGALAX, RDU2_PANEL_32P0),
};

/*
 * This initcall needs to be executed before coredevices, so we have a chance
 * to fix up the internal DT with the correct display information.
 */
static int rdu2_process_fixups(void)
{
	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	zii_process_lru_fixups(rdu2_lru_fixups);

	return 0;
}
postmmu_initcall(rdu2_process_fixups);
