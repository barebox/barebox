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
#include <generated/mach-types.h>
#include <linux/sizes.h>
#include <globalvar.h>
#include <asm/system_info.h>

#include <mach/core.h>
#include <mach/mbox.h>
#include <mach/platform.h>

#include "lowlevel.h"

struct rpi_priv;
struct rpi_machine_data {
	int (*init)(struct rpi_priv *priv);
	u8 hw_id;
#define RPI_OLD_SCHEMA			BIT(0)
	u8 flags;
};

struct rpi_priv {
	struct device_d *dev;
	const struct rpi_machine_data *dcfg;
	unsigned int hw_id;
	const char *name;
};

struct msg_get_arm_mem {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_arm_mem get_arm_mem;
	u32 end_tag;
};

struct msg_get_board_rev {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_board_rev get_board_rev;
	u32 end_tag;
};

struct msg_get_mac_address {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_mac_address get_mac_address;
	u32 end_tag;
};

static int rpi_get_arm_mem(u32 *size)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_arm_mem, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_arm_mem, GET_ARM_MEMORY);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret)
		return ret;

	*size = msg->get_arm_mem.body.resp.mem_size;

	return 0;
}

static void rpi_set_usbethaddr(void)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_mac_address, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_mac_address, GET_MAC_ADDRESS);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		printf("bcm2835: Could not query MAC address\n");
		/* Ignore error; not critical */
		return;
	}

	eth_register_ethaddr(0, msg->get_mac_address.body.resp.mac);
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

static int rpi_get_board_rev(struct rpi_priv *priv)
{
	int ret;

	BCM2835_MBOX_STACK_ALIGN(struct msg_get_board_rev, msg);
	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_board_rev, GET_BOARD_REV);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		dev_err(priv->dev, "Could not query board revision\n");
		return ret;
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
	priv->hw_id = msg->get_board_rev.body.resp.rev;

	return 0;
}

static int rpi_mem_init(void)
{
	u32 size = 0;
	int ret;

	ret = rpi_get_arm_mem(&size);
	if (ret)
		printf("could not query ARM memory size\n");

	bcm2835_add_device_sdram(size);

	return ret;
}
mem_initcall(rpi_mem_init);

static int rpi_env_init(void)
{
	struct stat s;
	const char *diskdev = "/dev/disk0.0";
	int ret;

	device_detect_by_name("mci0");

	ret = stat(diskdev, &s);
	if (ret) {
		printf("no %s. using default env\n", diskdev);
		return 0;
	}

	mkdir("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot", NULL);
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	defaultenv_append_directory(defaultenv_rpi);

	default_environment_path_set("/boot/barebox.env");

	return 0;
}

/* Extract /chosen/bootargs from the VideoCore FDT into vc.bootargs
 * global variable. */
static int rpi_vc_fdt_bootargs(void *fdt)
{
	int ret = 0;
	struct device_node *root = NULL, *node;
	const char *cmdline;

	root = of_unflatten_dtb(fdt, INT_MAX);
	if (IS_ERR(root)) {
		ret = PTR_ERR(root);
		root = NULL;
		goto out;
	}

	node = of_find_node_by_path_from(root, "/chosen");
	if (!node) {
		pr_err("no /chosen node\n");
		ret = -ENOENT;
		goto out;
	}

	cmdline = of_get_property(node, "bootargs", NULL);
	if (!cmdline) {
		pr_err("no bootargs property in the /chosen node\n");
		ret = -ENOENT;
		goto out;
	}

	globalvar_add_simple("vc.bootargs", cmdline);

	switch(cpu_architecture()) {
	case CPU_ARCH_ARMv6:
		globalvar_add_simple("vc.kernel", "kernel.img");
		break;
	case CPU_ARCH_ARMv7:
		globalvar_add_simple("vc.kernel", "kernel7.img");
		break;
	case CPU_ARCH_ARMv8:
		globalvar_add_simple("vc.kernel", "kernel7l.img");
		break;
	}

out:
	if (root)
		of_delete_node(root);

	return ret;
}

static void rpi_vc_fdt(void)
{
	void *saved_vc_fdt;
	struct fdt_header *oftree;
	unsigned long magic, size;
	int ret;

	/* VideoCore FDT was copied in PBL just above Barebox memory */
	saved_vc_fdt = (void *)(arm_mem_endmem_get());

	oftree = saved_vc_fdt;
	magic = be32_to_cpu(oftree->magic);

	if (magic == VIDEOCORE_FDT_ERROR) {
		if (oftree->totalsize)
			pr_err("there was an error copying fdt in pbl: %d\n",
					be32_to_cpu(oftree->totalsize));
		return;
	}

	if (magic != FDT_MAGIC)
		return;

	size = be32_to_cpu(oftree->totalsize);
	if (write_file("/vc.dtb", saved_vc_fdt, size)) {
		pr_err("failed to save videocore fdt to a file\n");
		return;
	}

	ret = rpi_vc_fdt_bootargs(saved_vc_fdt);
	if (ret) {
		pr_err("failed to extract bootargs from videocore fdt: %d\n",
									ret);
		return;
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

static int rpi_devices_probe(struct device_d *dev)
{
	const struct rpi_machine_data *dcfg;
	struct regulator *reg;
	struct rpi_priv *priv;
	const char *name, *ptr;
	char *hostname;
	int ret;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	ret = rpi_get_board_rev(priv);
	if (ret)
		goto free_priv;

	dcfg = rpi_get_dcfg(priv);
	if (IS_ERR(dcfg))
		goto free_priv;

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
	rpi_vc_fdt();

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
	}, {
		.hw_id = BCM2837B0_BOARD_REV_CM3_PLUS,
	}, {
		.hw_id = BCM2837B0_BOARD_REV_ZERO_2,
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

	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(rpi_of_match);

static struct driver_d rpi_board_driver = {
	.name = "board-rpi",
	.probe = rpi_devices_probe,
	.of_compatible = DRV_OF_COMPAT(rpi_of_match),
};
late_platform_driver(rpi_board_driver);
