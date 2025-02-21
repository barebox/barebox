// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer, Pengutronix
// SPDX-FileCopyrightText: 2015 PHYTEC Messtechnik GmbH

/* Author: Stefan Christ <s.christ@phytec.de> */

#define pr_fmt(fmt) "phySOM-i.MX6: " fmt

#include <malloc.h>
#include <envfs.h>
#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <of.h>
#include <deep-probe.h>
#include <i2c/i2c.h>
#include <mach/imx/bbu.h>
#include <platform_data/eth-fec.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <linux/micrel_phy.h>

#include <globalvar.h>

#include <mach/imx/iomux-mx6.h>
#include <mach/imx/imx6.h>

#define PHYFLEX_MODULE_REV_1	0x1
#define PHYFLEX_MODULE_REV_2	0x2

#define GPIO_2_11_PD_CTL	MX6_PAD_CTL_PUS_100K_DOWN | MX6_PAD_CTL_PUE | MX6_PAD_CTL_PKE | \
				MX6_PAD_CTL_SPEED_MED | MX6_PAD_CTL_DSE_40ohm | MX6_PAD_CTL_HYS

#define MX6Q_PAD_SD4_DAT3__GPIO_2_11_PD (_MX6Q_PAD_SD4_DAT3__GPIO_2_11 | MUX_PAD_CTRL(GPIO_2_11_PD_CTL))
#define MX6DL_PAD_SD4_DAT3__GPIO_2_11 IOMUX_PAD(0x0734, 0x034C, 5, 0x0000, 0, GPIO_2_11_PD_CTL)

#define MX6_PHYFLEX_ERR006282	IMX_GPIO_NR(2, 11)

#define DA9062_I2C_ADDRESS		0x58

#define DA9062_BUCK1_CFG		0x9e
#define DA9062_BUCK2_CFG		0x9d
#define DA9062_BUCK3_CFG		0xa0
#define DA9062_BUCK4_CFG		0x9f
#define DA9062_BUCKx_MODE_SYNCHRONOUS	(2 << 6)

static void phyflex_err006282_workaround(void)
{
	/*
	 * Boards beginning with 1362.2 have the SD4_DAT3 pin connected
	 * to the CMIC. If this pin isn't toggled within 10s the boards
	 * reset. The pin is unconnected on older boards, so we do not
	 * need a check for older boards before applying this fixup.
	 */

	gpio_direction_output(MX6_PHYFLEX_ERR006282, 0);
	mdelay(2);
	gpio_direction_output(MX6_PHYFLEX_ERR006282, 1);
	mdelay(2);
	gpio_set_value(MX6_PHYFLEX_ERR006282, 0);

	if (cpu_is_mx6q() || cpu_is_mx6d() || cpu_is_mx6qp() || cpu_is_mx6dp())
		mxc_iomux_v3_setup_pad(MX6Q_PAD_SD4_DAT3__GPIO_2_11_PD);
	else if (cpu_is_mx6dl() || cpu_is_mx6s())
		mxc_iomux_v3_setup_pad(MX6DL_PAD_SD4_DAT3__GPIO_2_11);

	gpio_direction_input(MX6_PHYFLEX_ERR006282);
}

static unsigned int pfla02_module_revision;

static unsigned int get_module_rev(void)
{
	unsigned int val = 0;

	val = gpio_get_value(IMX_GPIO_NR(2, 12));
	val |= (gpio_get_value(IMX_GPIO_NR(2, 13)) << 1);
	val |= (gpio_get_value(IMX_GPIO_NR(2, 14)) << 2);
	val |= (gpio_get_value(IMX_GPIO_NR(2, 15)) << 3);

	return 16 - val;
}

static int ksz8081_phy_fixup(struct phy_device *phydev)
{
	/*
	 * 0x8100 default
	 * 0x0080 RMII 50 MHz clock mode
	 * 0x0010 LED Mode 1
	 */
	phy_write(phydev, 0x1f, 0x8190);
	/*
	 * 0x0002 Override strap-in for RMII mode
	 * This should be default but after reset we occasionally read 0x0001
	 */
	phy_write(phydev, 0x16, 0x2);

	return 0;
}

static int phycore_da9062_setup_buck_mode(void)
{
	struct i2c_adapter *adapter = NULL;
	struct device_node *pmic_np = NULL;
	struct i2c_client client;
	unsigned char value;
	int ret;

	pmic_np = of_find_node_by_name_address(NULL, "pmic@58");
	if (!pmic_np)
		return -ENODEV;

	ret = of_device_ensure_probed(pmic_np);
	if (ret)
		return ret;

	adapter = of_find_i2c_adapter_by_node(pmic_np->parent);
	if (!adapter)
		return -ENODEV;

	client.adapter = adapter;
	client.addr = DA9062_I2C_ADDRESS;

	value = DA9062_BUCKx_MODE_SYNCHRONOUS;

	ret = i2c_write_reg(&client, DA9062_BUCK1_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK2_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK3_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK4_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	return 0;

err_out:
	return ret;
}

#define IS_PHYFLEX	BIT(0)
#define IS_PHYCORE	BIT(1)
#define IS_PHYCARD	BIT(2)
#define IS_PHYCORE_UL	BIT(3)
#define HAS_MMC3	BIT(4)
#define HAS_MMC1	BIT(5)

struct board_data {
	unsigned flags;
};

static int physom_imx6_probe(struct device *dev)
{
	int ret;
	char *environment_path, *default_environment_path;
	char *envdev, *default_envdev;
	const struct board_data *brd = device_get_match_data(dev);
	unsigned flags = brd->flags;

	if (flags & IS_PHYFLEX) {
		ret = of_devices_ensure_probed_by_property("gpio-controller");
		if (ret)
			return ret;

		phyflex_err006282_workaround();

		pfla02_module_revision = get_module_rev();
		globalvar_add_simple_int("board.revision", &pfla02_module_revision, "%u");
		pr_info("Module Revision: %u\n", pfla02_module_revision);

		barebox_set_hostname("phyFLEX-i.MX6");
		default_environment_path = "/chosen/environment-spinor";
		default_envdev = "SPI NOR flash";

		imx6_bbu_internal_mmc_register_handler("mmc2",
						"/dev/mmc2", 0);

	} else if (flags & IS_PHYCARD) {
		barebox_set_hostname("phyCARD-i.MX6");
		default_environment_path = "/chosen/environment-nand";
		default_envdev = "NAND flash";

		imx6_bbu_internal_mmc_register_handler("mmc2",
						"/dev/mmc2", 0);
	} else if (flags & IS_PHYCORE) {
		if (phycore_da9062_setup_buck_mode())
			pr_err("Setting PMIC BUCK mode failed\n");

		barebox_set_hostname("phyCORE-i.MX6");
		default_environment_path = "/chosen/environment-spinor";
		default_envdev = "SPI NOR flash";

		imx6_bbu_internal_mmc_register_handler("mmc0",
						"/dev/mmc0", 0);

	} else if (flags & IS_PHYCORE_UL) {
		barebox_set_hostname("phyCORE-i.MX6UL");
		default_environment_path = "/chosen/environment-nand";
		default_envdev = "NAND flash";

		phy_register_fixup_for_uid(PHY_ID_KSZ8081, MICREL_PHY_ID_MASK,
				ksz8081_phy_fixup);

		imx6_bbu_internal_mmc_register_handler("mmc0",
						"/dev/mmc0", 0);

	} else {
		return -EINVAL;
	}

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		environment_path = basprintf("/chosen/environment-sd%d",
					       bootsource_get_instance() + 1);
		envdev = "MMC";
		break;
	case BOOTSOURCE_NAND:
		environment_path = basprintf("/chosen/environment-nand");
		envdev = "NAND flash";
		break;
	case BOOTSOURCE_SPI_NOR:
		environment_path = basprintf("/chosen/environment-spinor");
		envdev = "SPI NOR flash";
		break;
	default:
		environment_path = strdup(default_environment_path);
		envdev = default_envdev;
		break;
	}

	if (environment_path) {
		ret = of_device_enable_path(environment_path);
		if (ret < 0)
			pr_warn("Failed to enable environment partition '%s' (%d)\n",
				environment_path, ret);
		free(environment_path);
	}

	pr_notice("Using environment in %s\n", envdev);

	if (flags & HAS_MMC3) {
		imx6_bbu_internal_mmc_register_handler("mmc3",
						"/dev/mmc3",
						BBU_HANDLER_FLAG_DEFAULT);
		imx6_bbu_internal_mmcboot_register_handler("mmc3-boot",
						"mmc3", 0);
	} else if (flags & HAS_MMC1) {
		imx6_bbu_internal_mmc_register_handler("mmc1",
						"/dev/mmc1",
						BBU_HANDLER_FLAG_DEFAULT);
		imx6_bbu_internal_mmcboot_register_handler("mmc1-boot",
						"mmc1", 0);
	} else {
		imx6_bbu_nand_register_handler("nand",
						"/dev/nand0.barebox",
						BBU_HANDLER_FLAG_DEFAULT);
	}

	defaultenv_append_directory(defaultenv_physom_imx6);

	/* Overwrite file /env/init/automount */
	if (flags & IS_PHYCARD || flags & IS_PHYFLEX) {
		defaultenv_append_directory(defaultenv_physom_imx6);
	} else if (flags & IS_PHYCORE) {
		defaultenv_append_directory(defaultenv_physom_imx6);
		defaultenv_append_directory(defaultenv_physom_imx6_phycore);
	} else if (flags & IS_PHYCORE_UL) {
		defaultenv_append_directory(defaultenv_physom_imx6ul_phycore);
	}

	return 0;
}

static struct board_data imx6q_pfla02 = {
	.flags = IS_PHYFLEX,
};

static struct board_data imx6dl_pfla02 = {
	.flags = IS_PHYFLEX,
};

static struct board_data imx6s_pfla02 = {
	.flags = IS_PHYFLEX,
};

static struct board_data imx6q_pcaaxl3 = {
	.flags = IS_PHYCARD,
};

static struct board_data imx6q_pcm058_nand = {
	.flags = IS_PHYCORE,
};

static struct board_data imx6q_pcm058_emmc = {
	.flags = IS_PHYCORE | HAS_MMC3,
};

static struct board_data imx6dl_pcm058_nand = {
	.flags = IS_PHYCORE,
};

static struct board_data imx6qp_pcm058_nand = {
	.flags = IS_PHYCORE,
};

static struct board_data imx6dl_pcm058_emmc = {
	.flags = IS_PHYCORE | HAS_MMC3,
};

static struct board_data imx6ul_pcl063_nand = {
	.flags = IS_PHYCORE_UL,
};

static struct board_data imx6ul_pcl063_emmc = {
	.flags = IS_PHYCORE_UL | HAS_MMC1,
};


static const struct of_device_id physom_imx6_match[] = {
	{
		.compatible = "phytec,imx6q-pfla02",
		.data = &imx6q_pfla02,
	}, {
		.compatible = "phytec,imx6dl-pfla02",
		.data = &imx6dl_pfla02,
	}, {
		.compatible = "phytec,imx6s-pfla02",
		.data = &imx6s_pfla02,
	}, {
		.compatible = "phytec,imx6q-pcaaxl3",
		.data = &imx6q_pcaaxl3,
	}, {
		.compatible = "phytec,imx6q-pcm058-nand",
		.data = &imx6q_pcm058_nand,
	}, {
		.compatible = "phytec,imx6q-pcm058-emmc",
		.data = &imx6q_pcm058_emmc,
	}, {
		.compatible = "phytec,imx6dl-pcm058-nand",
		.data = &imx6dl_pcm058_nand,
	}, {
		.compatible = "phytec,imx6qp-pcm058-nand",
		.data = &imx6qp_pcm058_nand,
	}, {
		.compatible = "phytec,imx6dl-pcm058-emmc",
		.data = &imx6dl_pcm058_emmc,
	}, {
		.compatible = "phytec,imx6ul-pcl063-nand",
		.data = &imx6ul_pcl063_nand,
	}, {
		.compatible = "phytec,imx6ul-pcl063-emmc",
		.data = &imx6ul_pcl063_emmc,
	},
	{ /* Sentinel */ },
};

static struct driver physom_imx6_driver = {
	.name = "physom-imx6",
	.probe = physom_imx6_probe,
	.of_compatible = physom_imx6_match,
};

postcore_platform_driver(physom_imx6_driver);

BAREBOX_DEEP_PROBE_ENABLE(physom_imx6_match);
