// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 David Jander, Protonic Holland
// SPDX-FileCopyrightText: 2021 Oleksij Rempel, Pengutronix

#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <mach/stm32mp/bbu.h>
#include <of_device.h>
#include <deep-probe.h>

/* board specific flags */
#define PRT_STM32_BOOTSRC_SD		BIT(2)
#define PRT_STM32_BOOTSRC_EMMC		BIT(1)
#define PRT_STM32_BOOTSRC_SPI_NOR	BIT(0)

struct prt_stm32_machine_data {
	u32 flags;
};

struct prt_stm32_boot_dev {
	char *name;
	char *env;
	char *dev;
	int flags;
	int boot_idx;
	enum bootsource boot_src;
};

static const struct prt_stm32_boot_dev prt_stm32_boot_devs[] = {
	{
		.name = "emmc",
		.env = "/chosen/environment-emmc",
		.dev = "/dev/mmc1.ssbl",
		.flags = PRT_STM32_BOOTSRC_EMMC,
		.boot_src = BOOTSOURCE_MMC,
		.boot_idx = 1,
	}, {
		.name = "qspi",
		.env = "/chosen/environment-qspi",
		.dev = "/dev/flash.ssbl",
		.flags = PRT_STM32_BOOTSRC_SPI_NOR,
		.boot_src = BOOTSOURCE_SPI_NOR,
		.boot_idx = -1,
	}, {
		/* SD is optional boot source and should be last device in the
		 * list. */
		.name = "sd",
		.env = "/chosen/environment-sd",
		.dev = "/dev/mmc0.ssbl",
		.flags = PRT_STM32_BOOTSRC_SD,
		.boot_src = BOOTSOURCE_MMC,
		.boot_idx = 0,
	},
};

static int prt_stm32_probe(struct device *dev)
{
	const struct prt_stm32_machine_data *dcfg;
	char *env_path_back = NULL, *env_path = NULL;
	int ret, i;

	dcfg = of_device_get_match_data(dev);
	if (!dcfg) {
		ret = -EINVAL;
		goto exit_get_dcfg;
	}

	for (i = 0; i < ARRAY_SIZE(prt_stm32_boot_devs); i++) {
		const struct prt_stm32_boot_dev *bd = &prt_stm32_boot_devs[i];
		int bbu_flags = 0;

		/* skip not supported boot sources */
		if (!(bd->flags & dcfg->flags))
			continue;

		/* first device is build-in device */
		if (!env_path_back)
			env_path_back = bd->env;

		if (bd->boot_src == bootsource_get() && (bd->boot_idx == -1 ||
		    bd->boot_idx  == bootsource_get_instance())) {
			bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
			env_path = bd->env;
		}

		ret = stm32mp_bbu_mmc_register_handler(bd->name, bd->dev,
						       bbu_flags);
		if (ret < 0)
			dev_warn(dev, "Failed to enable %s bbu (%pe)\n",
				 bd->name, ERR_PTR(ret));
	}

	if (!env_path)
		env_path = env_path_back;
	ret = of_device_enable_path(env_path);
	if (ret < 0)
		dev_warn(dev, "Failed to enable environment partition '%s' (%pe)\n",
			 env_path, ERR_PTR(ret));

	return 0;

exit_get_dcfg:
	dev_err(dev, "Failed to get dcfg: %pe\n", ERR_PTR(ret));
	return ret;
}

static const struct prt_stm32_machine_data prt_stm32_prtt1a = {
	.flags = PRT_STM32_BOOTSRC_SD | PRT_STM32_BOOTSRC_SPI_NOR,
};

static const struct prt_stm32_machine_data prt_stm32_prtt1c = {
	.flags = PRT_STM32_BOOTSRC_SD | PRT_STM32_BOOTSRC_EMMC,
};

static const struct prt_stm32_machine_data prt_stm32_mecio1 = {
	.flags = PRT_STM32_BOOTSRC_SPI_NOR,
};

static const struct prt_stm32_machine_data prt_stm32_mect1s = {
	.flags = PRT_STM32_BOOTSRC_SPI_NOR,
};

static const struct prt_stm32_machine_data prt_stm32_plyaqm = {
	.flags = PRT_STM32_BOOTSRC_SD | PRT_STM32_BOOTSRC_EMMC,
};

static const struct of_device_id prt_stm32_of_match[] = {
	{ .compatible = "prt,prtt1a", .data = &prt_stm32_prtt1a },
	{ .compatible = "prt,prtt1c", .data = &prt_stm32_prtt1c },
	{ .compatible = "prt,prtt1s", .data = &prt_stm32_prtt1a },
	{ .compatible = "prt,mecio1", .data = &prt_stm32_mecio1 },
	{ .compatible = "prt,mect1s", .data = &prt_stm32_mect1s },
	{ .compatible = "ply,plyaqm", .data = &prt_stm32_plyaqm },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(prt_stm32_of_match);

static struct driver prt_stm32_board_driver = {
	.name = "board-protonic-stm32",
	.probe = prt_stm32_probe,
	.of_compatible = prt_stm32_of_match,
};
postcore_platform_driver(prt_stm32_board_driver);
