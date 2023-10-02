// SPDX-License-Identifier: GPL-2.0

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>

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
	skov_imx8mp_init_storage(dev);

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
