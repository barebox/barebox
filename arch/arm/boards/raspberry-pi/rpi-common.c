// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Carlo Caione <carlo@carlocaione.org>

#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <fs.h>
#include <of.h>
#include <of_device.h>
#include <linux/stat.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <envfs.h>
#include <regulator.h>
#include <malloc.h>
#include <libfile.h>
#include <gpio.h>
#include <net.h>
#include <led.h>
#include <asm/armlinux.h>
#include <asm/barebox-arm.h>
#include <asm/mach-types.h>
#include <linux/sizes.h>
#include <globalvar.h>
#include <asm/system_info.h>
#include <reset_source.h>

#include <mach/bcm283x/core.h>
#include <mach/bcm283x/mbox.h>
#include <mach/bcm283x/platform.h>

#include <soc/bcm283x/wdt.h>

#include "lowlevel.h"

//https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#BOOT_ORDER
static const char * const boot_mode_names[] = {
	[0x0] = "unknown",
	[0x1] = "sd",
	[0x2] = "net",
	[0x3] = "rpiboot",
	[0x4] = "usbmsd",
	[0x5] = "usbc",
	[0x6] = "nvme",
	[0x7] = "http",
};

struct rpi_priv;
struct rpi_machine_data {
	int (*init)(struct rpi_priv *priv);
	u8 hw_id;
#define RPI_OLD_SCHEMA			BIT(0)
	u8 flags;
};

struct rpi_priv {
	struct device *dev;
	const struct rpi_machine_data *dcfg;
	unsigned int hw_id;
	const char *name;
};

struct rpi_property_fixup_data {
    const struct device_node* vc_node;
    const char *propname;
};

static void rpi_set_usbethaddr(void)
{
	u8 mac[ETH_ALEN];

	if (rpi_get_usbethaddr(mac))
		return; /* Ignore error; not critical */

	eth_register_ethaddr(0, mac);
}

static void rpi_set_usbotg(const char *alias)
{
	struct device_node *usb;

	usb = of_find_node_by_alias(NULL, alias);
	if (usb)
		of_property_write_string(usb, "dr_mode", "otg");
}

static struct gpio_led rpi_leds[] = {
	{
		.gpio	= -EINVAL,
		.led	= {
			.name = "ACT",
		},
	}, {
		.gpio	= -EINVAL,
		.led	= {
			.name = "PWR",
		},
	},
};

static void rpi_add_led(void)
{
	int i;
	struct gpio_led *l;

	for (i = 0; i < ARRAY_SIZE(rpi_leds); i++) {
		l = &rpi_leds[i];

		if (gpio_is_valid(l->gpio))
			led_gpio_register(l);
	}

	l = &rpi_leds[0];
	if (gpio_is_valid(l->gpio))
		led_set_trigger(LED_TRIGGER_HEARTBEAT, &l->led);
}

static int rpi_eth_init(struct rpi_priv *priv)
{
	rpi_set_usbethaddr();
	return 0;
}

static int rpi_b_init(struct rpi_priv *priv)
{
	rpi_leds[0].gpio = 16;
	rpi_leds[0].active_low = 1;
	rpi_set_usbethaddr();

	return 0;
}

static int rpi_b_plus_init(struct rpi_priv *priv)
{
	rpi_leds[0].gpio = 47;
	rpi_leds[1].gpio = 35;
	rpi_set_usbethaddr();

	return 0;
}

static int rpi_0_init(struct rpi_priv *priv)
{
	rpi_leds[0].gpio = 47;
	rpi_set_usbotg("usb0");

	return 0;
}

static int rpi_0_w_init(struct rpi_priv *priv)
{
	struct device_node *np;
	int ret;

	rpi_0_init(priv);

	np = of_find_node_by_path("/chosen");
	if (!np)
		return -ENODEV;

	if (!of_device_enable_and_register_by_alias("serial1"))
		return -ENODEV;

	ret = of_property_write_string(np, "stdout-path", "serial1:115200n8");
	if (ret)
		return ret;

	return of_device_disable_by_alias("serial0");
}

static int rpi_mem_init(void)
{
	ssize_t size;

	if (!of_machine_is_compatible("brcm,bcm2837") &&
	    !of_machine_is_compatible("brcm,bcm2835") &&
	    !of_machine_is_compatible("brcm,bcm2711") &&
	    !of_machine_is_compatible("brcm,bcm2836"))
		return 0;

	size = rpi_get_arm_mem();
	if (size < 0) {
		printf("could not query ARM memory size\n");
		size = get_ram_size((ulong *) BCM2835_SDRAM_BASE, SZ_128M);
	}

	bcm2835_add_device_sdram(size);

	return 0;
}
mem_initcall(rpi_mem_init);

static int rpi_env_init(void)
{
	struct stat s;
	const char *diskdev;
	int ret;

	defaultenv_append_directory(defaultenv_rpi);

	device_detect_by_name("mci0");
	device_detect_by_name("mci1");

	diskdev = "/dev/disk0.0";
	ret = stat(diskdev, &s);
	if (ret) {
		device_detect_by_name("mmc0");
		diskdev = "/dev/mmc0.0";
		ret = stat(diskdev, &s);
	}
	if (ret) {
		printf("no /dev/disk0.0 or /dev/mmc0.0. using default env\n");
		return 0;
	}

	mkdir("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot", NULL);
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	default_environment_path_set("/boot/barebox.env");

	return 0;
}

/* Some string properties in fdt passed to us from vc may be
 * malformed by not being null terminated, so just create and
 * return a fixed copy.
 */
static char *of_read_vc_string(struct device_node *node,
			       const char *prop_name)
{
	int len;
	const char *str;

	str = of_get_property(node, prop_name, &len);
	if (!str) {
		pr_warn("no property '%s' found in vc fdt's '%pOF' node\n",
			prop_name, node);
		return NULL;
	}
	return xstrndup(str, len);
}

static enum reset_src_type rpi_decode_pm_rsts(struct device_node *chosen,
					      struct device_node *bootloader)
{
	u32 pm_rsts;
	int ret;

	ret = of_property_read_u32(chosen, "pm_rsts", &pm_rsts);
	if (ret && bootloader)
		 ret = of_property_read_u32(bootloader, "rsts", &pm_rsts);

	if (ret) {
		pr_warn("'pm_rsts' value not found in vc fdt\n");
		return RESET_UKWN;
	}
	/*
	 * https://github.com/raspberrypi/linux/issues/932#issuecomment-93989581
	 */

	if (pm_rsts & PM_RSTS_HADPOR_SET)
		return RESET_POR;
	if (pm_rsts & PM_RSTS_HADDR_SET)
		return RESET_JTAG;
	if (pm_rsts & PM_RSTS_HADWR_SET)
		return RESET_WDG;
	if (pm_rsts & PM_RSTS_HADSR_SET)
		return RESET_RST;

	return RESET_UKWN;
}

static int rpi_vc_fdt_fixup(struct device_node *root, void *data)
{
	const struct device_node *vc_node = data;
	struct device_node *node;
	struct property *pp;

	node = of_create_node(root, vc_node->full_name);
	if (!node)
		return -ENOMEM;

	for_each_property_of_node(vc_node, pp)
		of_copy_property(vc_node, pp->name, node);

	return 0;
}

static struct device_node *register_vc_fixup(struct device_node *root,
					     const char *path)
{
	struct device_node *ret, *tmp;

	ret = of_find_node_by_path_from(root, path);
	if (ret) {
		tmp = of_dup(ret);
		tmp->full_name = xstrdup(ret->full_name);
		of_register_fixup(rpi_vc_fdt_fixup, tmp);
	} else {
		pr_debug("no '%s' node found in vc fdt\n", path);
	}

	return ret;
}

static int rpi_vc_fdt_fixup_property(struct device_node *root, void *data)
{
	const struct rpi_property_fixup_data *fixup = data;
	struct device_node *node;
	struct property *prop;

	node = of_create_node(root, fixup->vc_node->full_name);
	if (!node)
		return -ENOMEM;

	prop = of_find_property(fixup->vc_node, fixup->propname, NULL);
	if (!prop)
		return -ENOENT;

	return of_set_property(node, prop->name,
						   of_property_get_value(prop), prop->length, 1);
}

static int register_vc_property_fixup(struct device_node *root,
						 const char *path, const char *propname)
{
	struct device_node *node, *tmp;
	struct rpi_property_fixup_data* fixup_data;

	node = of_find_node_by_path_from(root, path);
	if (node) {
		tmp = of_dup(node);
		tmp->full_name = xstrdup(node->full_name);
		fixup_data = xzalloc(sizeof(*fixup_data));
		fixup_data->vc_node = tmp;
		fixup_data->propname = xstrdup(propname);

		of_register_fixup(rpi_vc_fdt_fixup_property, fixup_data);
	} else {
		pr_debug("no '%s' node found in vc fdt\n", path);
		return -ENOENT;
	}

	return 0;
}

static u32 rpi_boot_mode, rpi_boot_part;
/* Extract useful information from the VideoCore FDT we got.
 * Some parameters are defined here:
 * https://www.raspberrypi.com/documentation/computers/configuration.html#part4
 */
static void rpi_vc_fdt_parse(struct device_node *root)
{
	int ret;
	struct device_node *chosen, *bootloader, *memory;
	char *str;

	str = of_read_vc_string(root, "serial-number");
	if (str) {
		barebox_set_serial_number(str);
		free(str);
	}

	str = of_read_vc_string(root, "model");
	if (str) {
		barebox_set_model(str);
		free(str);
	}

	register_vc_fixup(root, "/system");
	register_vc_fixup(root, "/axi");
	register_vc_property_fixup(root, "/reserved-memory/nvram@0", "reg");
	register_vc_property_fixup(root, "/reserved-memory/nvram@0", "status");
	register_vc_fixup(root, "/hat");
	register_vc_property_fixup(root, "/emmc2bus", "dma-ranges");
	register_vc_fixup(root, "/chosen/bootloader");
	chosen = register_vc_fixup(root, "/chosen");
	if (!chosen) {
		pr_err("no '/chosen' node found in vc fdt\n");
		return;
	}

	bootloader = of_find_node_by_name(chosen, "bootloader");

	str = of_read_vc_string(chosen, "bootargs");
	if (str) {
		globalvar_add_simple("vc.bootargs", str);
		free(str);
	}

	str = of_read_vc_string(chosen, "overlay_prefix");
	if (str) {
		globalvar_add_simple("vc.overlay_prefix", str);
		free(str);
	}

	str = of_read_vc_string(chosen, "os_prefix");
	if (str) {
		globalvar_add_simple("vc.os_prefix", str);
		free(str);
	}

	ret = of_property_read_u32(chosen, "boot-mode", &rpi_boot_mode);
	if (ret && bootloader)
		ret = of_property_read_u32(bootloader, "boot-mode", &rpi_boot_mode);
	if (ret)
		pr_debug("'boot-mode' property not found in vc fdt\n");
	else
		globalvar_add_simple_enum("vc.boot_mode", &rpi_boot_mode,
					  boot_mode_names,
					  ARRAY_SIZE(boot_mode_names));

	ret = of_property_read_u32(chosen, "partition", &rpi_boot_part);
	if (ret && bootloader)
		ret = of_property_read_u32(bootloader, "partition", &rpi_boot_part);
	if (ret)
		pr_debug("'partition' property not found in vc fdt\n");
	else
		globalvar_add_simple_int("vc.boot_partition", &rpi_boot_part, "%u");

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		reset_source_set(rpi_decode_pm_rsts(chosen, bootloader));

	/* Parse all available nodes with "memory" device_type */
	memory = root;
	while (1) {
		memory = of_find_node_by_type(memory, "memory");
		if (!memory)
			break;

		of_add_memory(memory, false);
	}
}

/**
 * rpi_vc_fdt - unflatten VideoCore provided DT
 *
 * If configured via config.txt, the VideoCore firmware will pass barebox PBL
 * a device-tree in a register. This is saved to a handover memory area by
 * the Raspberry Pi PBL, which is parsed here. barebox-dt-2nd doesn't
 * populate this area, instead it uses the VideoCore DT as its own DT.
 *
 * Return: an unflattened DT on success, an error pointer if parsing the DT
 * fails and NULL if a Raspberry Pi PBL has run, but no VideoCore FDT was
 * saved.
 */
static struct device_node *rpi_vc_fdt(void)
{
	void *saved_vc_fdt;
	struct fdt_header *oftree;
	unsigned long magic, size;

	/* VideoCore FDT was copied in PBL just above Barebox memory */
	saved_vc_fdt = (void *)(arm_mem_endmem_get());

	oftree = saved_vc_fdt;
	magic = be32_to_cpu(oftree->magic);

	if (magic == VIDEOCORE_FDT_ERROR) {
		if (oftree->totalsize)
			pr_err("there was an error copying fdt in pbl: %d\n",
					be32_to_cpu(oftree->totalsize));
		return NULL;
	}

	if (magic != FDT_MAGIC)
		return ERR_PTR(-EINVAL);

	size = be32_to_cpu(oftree->totalsize);
	if (write_file("/vc.dtb", saved_vc_fdt, size))
		pr_err("failed to save videocore fdt to a file\n");

	return of_unflatten_dtb(saved_vc_fdt, INT_MAX);
}

static void rpi_set_kernel_name(void) {
	switch(cpu_architecture()) {
	case CPU_ARCH_ARMv6:
		globalvar_add_simple("vc.kernel", "kernel.img");
		break;
	case CPU_ARCH_ARMv7:
		globalvar_add_simple("vc.kernel", "kernel7.img");
		break;
	case CPU_ARCH_ARMv8:
		globalvar_add_simple("vc.kernel", "kernel8.img");
		break;
	}
}

static const struct rpi_machine_data *rpi_get_dcfg(struct rpi_priv *priv)
{
	const struct rpi_machine_data *dcfg;

	dcfg = of_device_get_match_data(priv->dev);
	if (!dcfg) {
		dev_err(priv->dev, "Unknown board. Not applying fixups\n");
		return NULL;
	}

	/* Comments from u-boot:
	 * For details of old-vs-new scheme, see:
	 * https://github.com/pimoroni/RPi.version/blob/master/RPi/version.py
	 * http://www.raspberrypi.org/forums/viewtopic.php?f=63&t=99293&p=690282
	 * (a few posts down)
	 *
	 * For the RPi 1, bit 24 is the "warranty bit", so we mask off just the
	 * lower byte to use as the board rev:
	 * http://www.raspberrypi.org/forums/viewtopic.php?f=63&t=98367&start=250
	 * http://www.raspberrypi.org/forums/viewtopic.php?f=31&t=20594
	 */

	for (; dcfg->hw_id != U8_MAX; dcfg++) {
		if (priv->hw_id & 0x800000) {
			if (dcfg->hw_id != ((priv->hw_id >> 4) & 0xff))
				continue;
		} else {
			if (!(dcfg->flags & RPI_OLD_SCHEMA))
				continue;
			if (dcfg->hw_id != (priv->hw_id & 0xff))
				continue;
		}

		return dcfg;
	}

	dev_err(priv->dev, "dcfg 0x%x for board_id doesn't match DT compatible\n",
		priv->hw_id);
	return ERR_PTR(-ENODEV);
}

static int rpi_devices_probe(struct device *dev)
{
	const struct rpi_machine_data *dcfg;
	struct regulator *reg;
	struct rpi_priv *priv;
	struct device_node *vc_root;
	const char *name, *ptr;
	char *hostname;
	int ret;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	ret = rpi_get_board_rev();
	if (ret < 0)
		goto free_priv;

	priv->hw_id = ret;

	dcfg = rpi_get_dcfg(priv);
	if (IS_ERR(dcfg)) {
		ret = PTR_ERR(dcfg);
		goto free_priv;
	}

	/* construct short recognizable host name */
	name = of_device_get_match_compatible(priv->dev);
	ptr = strchr(name, ',');
	hostname = basprintf("rpi-%s", ptr ? ptr + 1 : name);
	barebox_set_hostname(hostname);
	free(hostname);

	rpi_add_led();
	bcm2835_register_fb();
	armlinux_set_architecture(MACH_TYPE_BCM2708);
	rpi_env_init();

	vc_root = rpi_vc_fdt();
	if (!vc_root) {
		dev_dbg(dev, "No VideoCore FDT was provided\n");
	} else if (!IS_ERR(vc_root)) {
		dev_dbg(dev, "VideoCore FDT was provided\n");
		rpi_vc_fdt_parse(vc_root);
		of_delete_node(vc_root);
	} else if (IS_ERR(vc_root)) {
		/* This is intentionally at a higher logging level, because we can't
		 * be sure that the external DT is indeed a barebox DT (and not a
		 * kernel DT that happened to be in the partition). So for ease
		 * of debugging, we report this at info log level.
		 */
		dev_info(dev, "barebox FDT will be used for VideoCore FDT\n");
		rpi_vc_fdt_parse(priv->dev->device_node);
	}

	rpi_set_kernel_name();

	if (dcfg && dcfg->init)
		dcfg->init(priv);

	reg = regulator_get_name("bcm2835_usb");
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	regulator_enable(reg);

	return 0;

free_priv:
	kfree(priv);
	return ret;
}

static const struct rpi_machine_data rpi_1_ids[] = {
	{
		.hw_id = BCM2835_BOARD_REV_A_7,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_A_8,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_A_9,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_A,
	}, {
		.hw_id = BCM2835_BOARD_REV_A_PLUS_12,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_A_PLUS_15,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_A_PLUS,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_I2C1_4,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_I2C1_5,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_I2C1_6,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_B,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_I2C0_2,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_I2C0_3,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_REV2_d,
		.flags = RPI_OLD_SCHEMA,
		.init = rpi_b_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_REV2_e,
		.flags = RPI_OLD_SCHEMA,
		.init = rpi_b_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_REV2_f,
		.flags = RPI_OLD_SCHEMA,
		.init = rpi_b_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_PLUS_10,
		.flags = RPI_OLD_SCHEMA,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_PLUS_13,
		.flags = RPI_OLD_SCHEMA,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_B_PLUS,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_CM_11,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_CM_14,
		.flags = RPI_OLD_SCHEMA,
	}, {
		.hw_id = BCM2835_BOARD_REV_CM1,
	}, {
		.hw_id = BCM2835_BOARD_REV_ZERO,
		.init = rpi_0_init,
	}, {
		.hw_id = BCM2835_BOARD_REV_ZERO_W,
		.init = rpi_0_w_init,
	}, {
		.hw_id = U8_MAX
	},
};

static const struct rpi_machine_data rpi_2_ids[] = {
	{
		.hw_id = BCM2836_BOARD_REV_2_B,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = U8_MAX
	},
};

static const struct rpi_machine_data rpi_3_ids[] = {
	{
		.hw_id = BCM2837B0_BOARD_REV_3A_PLUS,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = BCM2837_BOARD_REV_3_B,
		.init = rpi_b_init,
	}, {
		.hw_id = BCM2837B0_BOARD_REV_3B_PLUS,
		.init = rpi_b_plus_init,
	}, {
		.hw_id = BCM2837_BOARD_REV_CM3,
		.init = rpi_eth_init,
	}, {
		.hw_id = BCM2837B0_BOARD_REV_CM3_PLUS,
	}, {
		.hw_id = BCM2837B0_BOARD_REV_ZERO_2,
	}, {
		.hw_id = U8_MAX
	},
};

static const struct rpi_machine_data rpi_4_ids[] = {
	{
		.hw_id = BCM2711_BOARD_REV_4_B,
		.init = rpi_eth_init,
	}, {
		.hw_id = BCM2711_BOARD_REV_400,
		.init = rpi_eth_init,
	}, {
		.hw_id = BCM2711_BOARD_REV_CM4,
		.init = rpi_eth_init,
	}, {
		.hw_id = BCM2711_BOARD_REV_CM4_S,
		.init = rpi_eth_init,
	}, {
		.hw_id = U8_MAX
	},
};

static const struct of_device_id rpi_of_match[] = {
	/* BCM2835 based Boards */
	{ .compatible = "raspberrypi,model-a", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-a-plus", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-b", .data = rpi_1_ids },
	/* Raspberry Pi Model B (no P5) */
	{ .compatible = "raspberrypi,model-b-i2c0", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-b-rev2", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-b-plus", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,compute-module", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-zero", .data = rpi_1_ids },
	{ .compatible = "raspberrypi,model-zero-w", .data = rpi_1_ids },

	/* BCM2836 based Boards */
	{ .compatible = "raspberrypi,2-model-b", .data = rpi_2_ids },

	/* BCM2837 based Boards */
	{ .compatible = "raspberrypi,3-model-a-plus", .data = rpi_3_ids },
	{ .compatible = "raspberrypi,3-model-b", .data = rpi_3_ids },
	{ .compatible = "raspberrypi,3-model-b-plus", .data = rpi_3_ids },
	{ .compatible = "raspberrypi,model-zero-2-w", .data = rpi_3_ids },
	{ .compatible = "raspberrypi,3-compute-module", .data = rpi_3_ids },
	{ .compatible = "raspberrypi,3-compute-module-lite", .data = rpi_3_ids },

	/* BCM2711 based Boards */
	{ .compatible = "raspberrypi,4-model-b", .data = rpi_4_ids },
	{ .compatible = "raspberrypi,4-compute-module", .data = rpi_4_ids },
	{ .compatible = "raspberrypi,4-compute-module-s", .data = rpi_4_ids },
	{ .compatible = "raspberrypi,400", .data = rpi_4_ids },

	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(rpi_of_match);

static struct driver rpi_board_driver = {
	.name = "board-rpi",
	.probe = rpi_devices_probe,
	.of_compatible = DRV_OF_COMPAT(rpi_of_match),
};
late_platform_driver(rpi_board_driver);
