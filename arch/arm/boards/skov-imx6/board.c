// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "skov-imx6: " fmt

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <gpio.h>
#include <init.h>
#include <linux/micrel_phy.h>
#include <mach/imx/bbu.h>
#include <net.h>
#include <of_gpio.h>

#include "version.h"

struct skov_imx6_priv {
	struct device *dev;
};

static struct skov_imx6_priv *skov_priv;

static int eth_of_fixup_node(struct device_node *root, const char *node_path,
			     const u8 *ethaddr)
{
	struct device_node *node;
	int ret;

	if (!is_valid_ether_addr(ethaddr)) {
		unsigned char ethaddr_str[sizeof("xx:xx:xx:xx:xx:xx")];

		ethaddr_to_string(ethaddr, ethaddr_str);
		dev_err(skov_priv->dev, "The mac-address %s is invalid.\n", ethaddr_str);
		return -EINVAL;
	}

	node = of_find_node_by_path_from(root, node_path);
	if (!node) {
		dev_err(skov_priv->dev, "Did not find node %s to fix up with stored mac-address.\n",
		        node_path);
		return -ENOENT;
	}

	ret = of_set_property(node, "mac-address", ethaddr, ETH_ALEN, 1);
	if (ret)
		dev_err(skov_priv->dev, "Setting mac-address property of %pOF failed with: %s.\n",
		        node, strerror(-ret));

	return ret;
}

static int eth_of_fixup_node_from_eth_device(struct device_node *root,
					     const char *node_path,
					     const char *ethname)
{
	struct eth_device *edev;

	edev = eth_get_byname(ethname);
	if (!edev) {
		dev_err(skov_priv->dev, "Did not find eth device \"%s\" to copy mac-address from.\n", ethname);
		return -ENOENT;
	}

	return eth_of_fixup_node(root, node_path, edev->ethaddr);
}

static int get_mac_address_from_env_variable(const char *env, u8 ethaddr[ETH_ALEN])
{
	const char *ethaddr_str;
	int ret;

	ethaddr_str = getenv(env);
	if (!ethaddr_str) {
		dev_err(skov_priv->dev, "State variable %s storing the mac-address not found.\n", env);
		return -ENOENT;
	}

	ret = string_to_ethaddr(ethaddr_str, ethaddr);
	if (ret < 0) {
		dev_err(skov_priv->dev, "Could not convert \"%s\" in state variable %s into mac-address.\n",
		        ethaddr_str, env);
		return -EINVAL;
	}

	return 0;
}

static int get_default_mac_address_from_state_node(const char *state_node_path,
						   u8 ethaddr[ETH_ALEN])
{
	struct device_node *node;
	int ret;

	node = of_find_node_by_path(state_node_path);
	if (!node) {
		dev_err(skov_priv->dev, "Node %s defining the state variable not found.\n", state_node_path);
		return -ENOENT;
	}

	ret = of_property_read_u8_array(node, "default", ethaddr, ETH_ALEN);
	if (ret) {
		dev_err(skov_priv->dev, "Node %s has no property \"default\" of proper type.\n", state_node_path);
		return -ENOENT;
	}

	return 0;
}

static int eth2_of_fixup_node_individually(struct device_node *root,
					   const char *node_path,
					   const char *ethname,
					   const char *env,
					   const char *state_node_path)
{
	u8 env_ethaddr[ETH_ALEN], default_ethaddr[ETH_ALEN];
	int ret;

	ret = get_mac_address_from_env_variable(env, env_ethaddr);
	if (ret)
		goto copy_mac_from_eth0;

	ret = get_default_mac_address_from_state_node(state_node_path, default_ethaddr);
	if (ret)
		goto copy_mac_from_eth0;

	/*
	 * As the default is bogus copy the MAC address from eth0 if
	 * the state variable has not been set to a different variant
	 */
	if (memcmp(env_ethaddr, default_ethaddr, ETH_ALEN) == 0)
		goto copy_mac_from_eth0;

	return eth_of_fixup_node(root, node_path, env_ethaddr);

copy_mac_from_eth0:
	return eth_of_fixup_node_from_eth_device(root, node_path, ethname);
}

#define MAX_V_GPIO 8

struct board_description {
	const char *variant;
	const char *revision;
	const char *soc;
	const char *dts_compatible;
	const char *display;
	unsigned flags;
};

#define SKOV_NEED_ENABLE_RMII	BIT(0)
#define SKOV_DISPLAY_PARALLEL	BIT(1)
#define SKOV_ENABLE_MMC_POWER	BIT(2)
#define SKOV_DISPLAY_LVDS	BIT(3)

static const struct board_description imx6_variants[] = {
	[0] = {
		.variant = "high performance",
		.revision = "A",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6-imxq-revA",
		.flags = SKOV_NEED_ENABLE_RMII | SKOV_DISPLAY_PARALLEL,
	},
	[1] = {
		.variant = "low cost",
		.revision = "A",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6-imxdl-revA",
		.flags = SKOV_NEED_ENABLE_RMII | SKOV_DISPLAY_PARALLEL,
	},
	[2] = {
		.variant = "high performance",
		.revision = "A",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6-imxq-revA",
		.flags = SKOV_NEED_ENABLE_RMII | SKOV_DISPLAY_PARALLEL,
	},
	[4] = {
		.variant = "low cost",
		.revision = "A",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6-imxdl-revA",
		.flags = SKOV_NEED_ENABLE_RMII | SKOV_DISPLAY_PARALLEL,
	},
	[8] = {
		.variant = "high performance",
		.revision = "A",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6-imxq-revA",
		.flags = SKOV_NEED_ENABLE_RMII | SKOV_DISPLAY_PARALLEL,
	},
	[9] = {
		.variant = "minimum cost",
		.revision = "B",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6-imxdl-revB",
		.flags = SKOV_DISPLAY_PARALLEL,
	},
	[10] = {
		.variant = "low cost",
		.revision = "B",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6-imxdl-revB",
		.flags = SKOV_DISPLAY_PARALLEL,
	},
	[11] = {
		.variant = "high performance",
		.revision = "B",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6-imxq-revB",
		.flags = SKOV_DISPLAY_PARALLEL,
	},
	[12] = {
		/* FIXME this one is a revision 'C' according to the schematics */
		.variant = "max performance",
		.revision = "B",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_DISPLAY_PARALLEL,
	},
	[13] = {
		.variant = "low cost",
		.revision = "C",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6dl-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[14] = {
		.variant = "high performance",
		.revision = "C",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[15] = {
		.variant = "middle performance",
		.revision = "C",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6dl-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[16] = {
		.variant = "Solo_R512M_F4G",
		.revision = "C",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6dl-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[17] = {
		.variant = "Quad_R2G_F8G",
		.revision = "C",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[18] = {
		.variant = "QuadPlus_R4G_F16G",
		.revision = "C",
		.soc = "i.MX6Q+",
		.dts_compatible = "skov,imx6q-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[19] = {
		.variant = "Solo_R512M_F2G",
		.revision = "C",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6dl-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[20] = {
		.variant = "Quad_R1G_F4G",
		.revision = "C",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-revc-lt2",
		.display = "l2rt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[21] = {
		.variant = "Solo_R512M_F2G",
		.revision = "C",
		.soc = "i.MX6S",
		.dts_compatible = "skov,imx6dl-skov-revc-lt6",
		.display = "l6whrt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[22] = {
		.variant = "Quad_R1G_F4G",
		.revision = "C",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-revc-lt6",
		.display = "l6whrt",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_PARALLEL,
	},
	[24] = {
		.variant = "Quad_R1G_F4G",
		.revision = "E",
		.soc = "i.MX6Q",
		.dts_compatible = "skov,imx6q-skov-reve-mi1010ait-1cp1",
		.display = "mi1010ait-1cp1",
		.flags = SKOV_ENABLE_MMC_POWER | SKOV_DISPLAY_LVDS,
	},
};

static int skov_board_no = -1;
static bool skov_have_switch = true;
static const char *no_switch_suffix = "-noswitch";

static void fixup_noswitch_machine_compatible(struct device_node *root)
{
	const char *compat = imx6_variants[skov_board_no].dts_compatible;
	const char *generic = "skov,imx6";
	char *buf;

	/* add generic compatible, so systemd&co can make right decisions */
	buf = xasprintf("%s%s", generic, no_switch_suffix);
	of_prepend_machine_compatible(root, buf);

	/* add specific compatible as fallback, in case this board has new
	 * challenges.
	 */
	buf = xasprintf("%s%s", compat, no_switch_suffix);
	of_prepend_machine_compatible(root, buf);

	free(buf);
}

static void skov_imx6_no_switch(struct device_node *root)
{
	const char *fec_alias = "ethernet0";
	struct device_node *node;
	int ret;

	fixup_noswitch_machine_compatible(root);

	node = of_find_node_by_alias(root, fec_alias);
	if (node) {
		ret = of_device_disable(node);
		if (ret)
			dev_warn(skov_priv->dev, "Can't disable %s\n", fec_alias);
	} else {
		dev_warn(skov_priv->dev, "Can't find node by alias: %s\n", fec_alias);
	}

	node = of_find_node_by_alias(root, "mdio-gpio0");
	if (node) {
		ret = of_device_disable(node);
		if (ret)
			dev_warn(skov_priv->dev, "Can't disable mdio-gpio0 node\n");
	} else {
		dev_warn(skov_priv->dev, "Can't find mdio-gpio0 node\n");
	}
}

static int skov_imx6_switch_port(struct device_node *root, const char *path)
{
	size_t size;
	char *buf;
	int ret;

	/* size is, string + '\0' + port number */
	size = strlen(path) + 2;
	buf = xzalloc(size);
	if (!buf)
		return -ENOMEM;

	ret = snprintf(buf, size, "%s0", path);
	if (ret < 0)
		return ret;

	ret = eth_of_fixup_node_from_eth_device(root, buf, "eth0");
	if (ret)
		return ret;

	ret = snprintf(buf, size, "%s1", path);
	if (ret < 0)
		return ret;

	ret = eth2_of_fixup_node_individually(root, buf, "eth0",
					      "state.ethaddr.eth2",
					      "/state/ethaddr/eth2");
	return ret;
}

static void skov_imx6_switch(struct device_node *root)
{
	const char *old = "/mdio-gpio/ksz8873@3/ports/ports@";
	const char *new = "/mdio/switch@0/ports/ports@";
	int ret;

	/* Old DTS variants (pre kernel mainline) use different path. Try first
	 * the new variant, then fall back to the old one.
	 */
	ret = skov_imx6_switch_port(root, new);
	if (ret) {
		ret = skov_imx6_switch_port(root, old);
		if (ret)
			dev_err(skov_priv->dev, "Filed to set mac address\n");
	}
}

static int skov_imx6_fixup(struct device_node *root, void *unused)
{
	struct device_node *chosen = of_create_node(root, "/chosen");
	struct device_node *node;
	uint32_t brightness;
	const char *val;
	int ret;

	if (skov_have_switch)
		skov_imx6_switch(root);
	else
		skov_imx6_no_switch(root);

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		/* use default variant of state variable defined in devicetree */
		brightness = 8;
		break;
	default:
		val = getenv("state.display.brightness");
		if (!val) {
			dev_err(skov_priv->dev, "could not get default display brightness\n");
			return 0;
		}

		brightness = simple_strtoul(val, NULL, 0);
		break;
	}

	for_each_compatible_node_from(node, root, NULL, "pwm-backlight") {
		ret = of_property_write_u32(node, "default-brightness-level", brightness);
		if (ret)
			dev_err(skov_priv->dev, "error %d while setting default-brightness-level property on node %s to %d\n",
				ret, node->name, brightness);
	}

	of_property_write_u32(chosen, "skov,imx6-board-version", skov_board_no);
	of_property_write_string(chosen, "skov,imx6-board-variant",
				 imx6_variants[skov_board_no].variant);

	return 0;
}

static void skov_init_parallel_lcd(void)
{
	struct device_node *lcd;

	lcd = of_find_compatible_node(NULL, NULL, "fsl,imx-parallel-display");
	if (!lcd) {
		dev_err(skov_priv->dev, "Cannot find \"fsl,imx-parallel-display\" node\n");
		return;
	}

	of_device_enable_and_register(lcd);
}

static void skov_init_ldb(void)
{
	struct device_node *ldb, *chan;

	ldb = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ldb");
	if (!ldb) {
		dev_err(skov_priv->dev, "Cannot find \"fsl,imx6q-ldb\" node\n");
		return;
	}

	/* First enable channel 0, prior to enabling parent */
	chan = of_find_node_by_name_address(ldb, "lvds-channel@0");
	if (chan)
		of_device_enable(chan);
	else
		dev_err(skov_priv->dev, "Cannot find \"lvds-channel@0\" node\n");

	/* Now probe will see the expected device tree */
	of_device_enable_and_register(ldb);
}

/*
 * Some variants need tweaks to make them work
 *
 * Revision A has no backlight control, since revision B it is present (GPIO6/23)
 * Revision A needs GPIO1/24 to be low to make network working
 * Revision C can control the SD main power supply
 */
static void skov_init_board(const struct board_description *variant)
{
	struct device_node *gpio_np = NULL;
	char *environment_path, *envdev;
	int ret;

	gpio_np = of_find_node_by_name_address(NULL, "gpio@20b4000");
	if (gpio_np) {
		ret = of_device_ensure_probed(gpio_np);
		if (ret)
			dev_warn(skov_priv->dev, "Can't probe GPIO node\n");
	} else {
		dev_warn(skov_priv->dev, "Can't get GPIO node\n");
	}

	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0.barebox",
		BBU_HANDLER_FLAG_DEFAULT);

	of_register_fixup(skov_imx6_fixup, NULL);

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		environment_path = "/chosen/environment-sd";
		envdev = "MMC";
		break;
	default:
		environment_path = "/chosen/environment-spinor";
		envdev = "SPI NOR flash";
		break;
	}

	dev_notice(skov_priv->dev, "Using environment in %s\n", envdev);

	ret = of_device_enable_path(environment_path);
	if (ret < 0)
		dev_warn(skov_priv->dev, "Failed to enable environment partition '%s' (%d)\n",
			 environment_path, ret);

	if (variant->flags & SKOV_NEED_ENABLE_RMII) {
		/*
		 * MX6QDL_PAD_ENET_RX_ER__GPIO1_IO24 is a gpio which must be
		 * low to enable the RMII from the switch point of view
		 */
		gpio_request(24, "must_be_low");
		gpio_direction_output(24, 0);
		gpio_free(24);
	}

	/* SD card handling */
	gpio_request(205, "mmc io supply");
	gpio_direction_output(205, 0); /* select 3.3 V IO voltage */
	gpio_free(205);

	if (variant->flags & SKOV_ENABLE_MMC_POWER) {
		/*
		 * keep in sync with devicetree's 'regulator-boot-on' setting for
		 * this regulator
		 */
		gpio_request(200, "mmc power supply");
		gpio_direction_output(200, 0); /* switch on */
		mdelay(1);
		gpio_direction_output(200, 1); /* switch on */
		gpio_free(200);
	}

	if (variant->flags & SKOV_DISPLAY_PARALLEL)
		skov_init_parallel_lcd();

	if (variant->flags & SKOV_DISPLAY_LVDS)
		skov_init_ldb();
}

static int skov_set_switch_lan2_mac(struct skov_imx6_priv *priv)
{
	const char *state = "/state/ethaddr/eth2";
	struct device_node *lan2_np;
	u8 ethaddr[ETH_ALEN];
	int ret;

	ret = get_mac_address_from_env_variable("state.ethaddr.eth2", ethaddr);
	if (ret || !is_valid_ether_addr(ethaddr)) {
		ret = get_default_mac_address_from_state_node(state, ethaddr);
		if (ret || !is_valid_ether_addr(ethaddr)) {
			dev_err(priv->dev, "can't get MAC for LAN2\n");
			return -ENODEV;
		}
	}

	lan2_np = of_find_node_by_path("/mdio/switch@0/ports/ports@1");
	if (!lan2_np) {
		dev_err(priv->dev, "LAN2 node not found\n");
		return -ENODEV;
	}

	of_eth_register_ethaddr(lan2_np, ethaddr);

	return 0;
}

static int skov_switch_test(void)
{
	struct device *sw_dev;
	struct device *eth0;
	int ret;

	if (skov_board_no < 0)
		return 0;

	/* Driver should be able to detect if device do actually
	 * exist. So, we need only to detect if driver is actually
	 * probed.
	 */
	sw_dev = of_find_device_by_node_path("/mdio/switch@0");
	if (!sw_dev) {
		dev_err(skov_priv->dev, "switch@0 device was not created!\n");
		goto no_switch;
	}

	if (dev_is_probed(sw_dev)) {
		skov_set_switch_lan2_mac(skov_priv);
		/* even if we fail, continue to boot as good as possible */
		return 0;
	}

no_switch:
	skov_have_switch = false;

	dev_notice(skov_priv->dev, "No-switch variant is detected\n");

	eth0 = get_device_by_name("eth0");
	if (eth0) {
		ret = dev_set_param(eth0, "mode", "disabled");
		if (ret)
			dev_warn(skov_priv->dev, "Can't set eth0 mode\n");
	} else {
		dev_warn(skov_priv->dev, "Can't disable eth0\n");
	}

	return 0;
}
late_initcall(skov_switch_test);

static int skov_imx6_probe(struct device *dev)
{
	struct skov_imx6_priv *priv;
	unsigned v = 0;
	const struct board_description *variant;

	v = skov_imx6_get_version();

	if (v >= ARRAY_SIZE(imx6_variants)) {
		dev_err(dev, "Invalid variant %u\n", v);
		return -EINVAL;
	}

	variant = &imx6_variants[v];

	if (!variant->variant) {
		dev_err(dev, "Invalid variant %u\n", v);
		return -EINVAL;
	}

	skov_board_no = v;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;
	skov_priv = priv;

	globalvar_add_simple_int("board.no", &skov_board_no, "%u");
	globalvar_add_simple("board.variant", variant->variant);
	globalvar_add_simple("board.revision",variant->revision);
	globalvar_add_simple("board.soc", variant->soc);
	globalvar_add_simple("board.dts", variant->dts_compatible);
	globalvar_add_simple("board.display", variant->display ?: NULL);

	of_prepend_machine_compatible(NULL, variant->dts_compatible);

	skov_init_board(variant);

	defaultenv_append_directory(defaultenv_skov_imx6);

	return 0;
}

static __maybe_unused struct of_device_id skov_version_ids[] = {
	{
		.compatible = "skov,imx6",
	}, {
		/* sentinel */
	}
};
BAREBOX_DEEP_PROBE_ENABLE(skov_version_ids);

static struct driver skov_version_driver = {
	.name = "skov-imx6",
	.probe = skov_imx6_probe,
	.of_compatible = DRV_OF_COMPAT(skov_version_ids),
};
coredevice_platform_driver(skov_version_driver);

static void skov_imx6_devices_shutdown(void)
{
	const char *external;

	if (skov_board_no < 0)
		return;

	external = getenv("state.display.external");
	if (!external) {
		dev_err(skov_priv->dev, "could not get state variable display.external\n");
		return;
	}

	if (!strcmp(external, "0"))
		setenv("backlight0.brightness", "0");
}
predevshutdown_exitcall(skov_imx6_devices_shutdown);
