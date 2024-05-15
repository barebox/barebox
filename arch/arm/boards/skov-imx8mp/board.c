// SPDX-License-Identifier: GPL-2.0

#include "linux/kernel.h"
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <mach/imx/bbu.h>
#include <mach/imx/generic.h>
#include <mach/imx/iomux-mx8mp.h>

struct skov_imx8mp_priv {
	struct device *dev;
	int variant_id;
};

static struct skov_imx8mp_priv *skov_imx8mp_priv;

#define GPIO_HW_VARIANT  {\
	{IMX_GPIO_NR(1, 8), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var0"}, \
	{IMX_GPIO_NR(1, 9), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var1"}, \
	{IMX_GPIO_NR(1, 10), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var2"}, \
	{IMX_GPIO_NR(1, 11), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var3"}, \
	{IMX_GPIO_NR(1, 12), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var4"}, \
	{IMX_GPIO_NR(1, 13), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var5"}, \
	{IMX_GPIO_NR(1, 14), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var6"}, \
	{IMX_GPIO_NR(1, 15), GPIOF_DIR_IN | GPIOF_ACTIVE_HIGH, "var7"}, \
}

struct skov_imx8mp_storage {
	const char *name;
	const char *env_path;
	const char *dev_path;
	enum bootsource bootsource;
	int bootsource_ext_id;
	bool mmc_boot_part;
};

enum skov_imx8mp_boot_source {
	SKOV_BOOT_SOURCE_EMMC,
	SKOV_BOOT_SOURCE_SD,
	SKOV_BOOT_SOURCE_UNKNOWN,
};

static const struct skov_imx8mp_storage skov_imx8mp_storages[] = {
	[SKOV_BOOT_SOURCE_EMMC] = {
		/* default boot source */
		.name = "eMMC",
		.env_path = "/chosen/environment-emmc",
		.dev_path = "/dev/mmc2",
		.bootsource = BOOTSOURCE_MMC,
		.bootsource_ext_id = 2,
		.mmc_boot_part = true,
	},
	[SKOV_BOOT_SOURCE_SD] = {
		.name = "SD",
		.env_path = "/chosen/environment-sd",
		.dev_path = "/dev/mmc1.barebox",
		.bootsource = BOOTSOURCE_MMC,
		.bootsource_ext_id = 1,
	},
};

struct board_description {
	const char *dts_compatible;
	const char *dts_compatible_hdmi;
	unsigned flags;
};

#define SKOV_IMX8MP_HAS_HDMI	BIT(0)

static const struct board_description imx8mp_variants[] = {
	[0] = {
		.dts_compatible = "skov,imx8mp-skov-revb-lt6",
	},
	[1] = {
		.dts_compatible = "skov,imx8mp-skov-revb-mi1010ait-1cp1",
		.dts_compatible_hdmi = "skov,imx8mp-skov-revb-hdmi",
		.flags = SKOV_IMX8MP_HAS_HDMI,
	},
	[2] = {
		.dts_compatible = "skov,imx8mp-skov-revc-bd500",
	},
};

static const struct board_description imx8mp_basic_variant = {
	.dts_compatible = "skov,imx8mp-skov-basic",
};

static int skov_imx8mp_fixup(struct device_node *root, void *data)
{
	struct device_node *chosen = of_create_node(root, "/chosen");
	const char *of_board = "skov,imx8mp-board-version";
	struct skov_imx8mp_priv *priv = data;
	struct device *dev = priv->dev;
	int ret;

	ret = of_property_write_u32(chosen, of_board, priv->variant_id);
	if (ret)
		dev_err(dev,  "Failed to fixup %s: %pe\n", of_board,
			ERR_PTR(ret));

	return 0;
}

static int skov_imx8mp_get_variant_id(uint *id)
{
	struct gpio gpios_rev[] = GPIO_HW_VARIANT;
	struct device_node *gpio_np;
	u32 hw_rev;
	int ret;

	gpio_np = of_find_node_by_name_address(NULL, "gpio@30200000");
	if (!gpio_np)
		return -ENODEV;

	ret = of_device_ensure_probed(gpio_np);
	if (ret)
		return ret;

	ret = gpio_array_to_id(gpios_rev, ARRAY_SIZE(gpios_rev), &hw_rev);
	if (ret)
		goto exit_get_id;

	*id = hw_rev;

	return 0;
exit_get_id:
	pr_err("Failed to read gpio ID: %pe\n", ERR_PTR(ret));
	return ret;
}

static int skov_imx8mp_get_hdmi(struct device *dev)
{
	const char *env = "state.display.external";
	struct device_node *state_np;
	unsigned int val = 0;
	int ret;

	state_np = of_find_node_by_name_address(NULL, "state");
	if (!state_np) {
		dev_err(dev, "Failed to find state node\n");
		return -ENODEV;
	}

	ret = of_device_ensure_probed(state_np);
	if (ret) {
		dev_err(dev, "Failed to probe state node: %pe\n", ERR_PTR(ret));
		return ret;
	}

	ret = getenv_uint(env, &val);
	if (ret) {
		dev_err(dev, "Failed to read %s: %pe\n", env, ERR_PTR(ret));
		return ret;
	}

	return val;
}

static int skov_imx8mp_init_variant(struct skov_imx8mp_priv *priv)
{
	const struct board_description *variant;
	struct device *dev = priv->dev;
	const char *compatible;
	unsigned int v = 0;
	int ret;

	ret = skov_imx8mp_get_variant_id(&v);
	if (ret)
		return ret;

	priv->variant_id = v;

	if (v >= ARRAY_SIZE(imx8mp_variants)) {
		dev_warn(dev, "Unsupported variant %u. Fall back to basic variant\n", v);
		variant = &imx8mp_basic_variant;
	} else {
		variant = &imx8mp_variants[v];
	}

	if (variant->flags & SKOV_IMX8MP_HAS_HDMI) {
		ret = skov_imx8mp_get_hdmi(dev);
		if (ret == 1)
			compatible = variant->dts_compatible_hdmi;
		else
			compatible = variant->dts_compatible;
	} else {
		compatible = variant->dts_compatible;
	}

	of_prepend_machine_compatible(NULL, compatible);

	return 0;
}

static void skov_imx8mp_enable_env(struct device *dev,
				   const struct skov_imx8mp_storage *st,
				   bool *enabled)
{
	int ret;

	if (bootsource_get() != st->bootsource ||
	    bootsource_get_instance() != st->bootsource_ext_id)
		return;

	ret = of_device_enable_path(st->env_path);
	if (ret) {
		dev_err(dev, "Failed to enable environment path: %s, %pe\n",
			st->env_path, ERR_PTR(ret));
		return;
	}

	*enabled = true;
}

static void skov_imx8mp_add_bbu(struct device *dev,
				const struct skov_imx8mp_storage *st,
				bool default_env)
{
	unsigned long flags = 0;
	int ret;

	if (default_env)
		flags |= BBU_HANDLER_FLAG_DEFAULT;

	if (st->mmc_boot_part) {
		ret = imx8m_bbu_internal_mmcboot_register_handler(st->name,
								  st->dev_path,
								  flags);
	} else {
		ret = imx8m_bbu_internal_mmc_register_handler(st->name,
							      st->dev_path,
							      flags);
	}
	if (ret)
		dev_err(dev, "Failed to register %s BBU handler: %pe\n",
			st->name, ERR_PTR(ret));
}

static void skov_imx8mp_init_storage(struct device *dev)
{
	int default_boot_src = SKOV_BOOT_SOURCE_EMMC;
	int i;

	for (i = 0; i < ARRAY_SIZE(skov_imx8mp_storages); i++) {
		bool enabled_env = false;

		skov_imx8mp_enable_env(dev, &skov_imx8mp_storages[i],
				       &enabled_env);
		if (enabled_env)
			default_boot_src = i;
	}

	for (i = 0; i < ARRAY_SIZE(skov_imx8mp_storages); i++)
		skov_imx8mp_add_bbu(dev, &skov_imx8mp_storages[i],
				    i == default_boot_src);
}

static int skov_imx8mp_probe(struct device *dev)
{
	struct skov_imx8mp_priv *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;
	skov_imx8mp_priv = priv;

	skov_imx8mp_init_storage(dev);

	skov_imx8mp_init_variant(priv);

	ret = of_register_fixup(skov_imx8mp_fixup, priv);
	if (ret)
		dev_err(dev, "Failed to register fixup: %pe\n", ERR_PTR(ret));

	return 0;
}

static const struct of_device_id skov_imx8mp_of_match[] = {
	/* generic, barebox specific compatible for all board variants */
	{ .compatible = "skov,imx8mp" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(skov_imx8mp_of_match);

static struct driver skov_imx8mp_board_driver = {
	.name = "skov-imx8m",
	.probe = skov_imx8mp_probe,
	.of_compatible = skov_imx8mp_of_match,
};
coredevice_platform_driver(skov_imx8mp_board_driver);
