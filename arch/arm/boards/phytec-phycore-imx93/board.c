// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "phyCORE imx93: " fmt

#include <common.h>
#include <init.h>
#include <linux/kernel.h>
#include <environment.h>
#include <mfd/pca9450.h>
#include <deep-probe.h>
#include <mach/imx/bbu.h>
#include <linux/pinctrl/consumer.h>

static void phycore_imx93_init_pmic(struct regmap *map)
{
	/* set WDOG_B_CFG to cold reset */
	regmap_write(map, PCA9450_RESET_CTRL, 0xA1);
}

static int phycore_imx93_probe(struct device *dev)
{
	struct device_node *np;

	pca9450_register_init_callback(phycore_imx93_init_pmic);

	/*
	 * The phy on the EQOS has its MDIO lines connected to the FEC. The phy
	 * registers can only be successfully read when the EQOS pinctrl setup
	 * has been done. The phys on the FEC MDIO bus are probed during the
	 * FEC driver probe, so do the EQOS pinctrl setup here to make sure it's
	 * done before the FEC probes.
	 */
	np = of_find_compatible_node(dev->of_node, NULL, "nxp,imx93-dwmac-eqos");
	BUG_ON(!np);
	of_pinctrl_select_state_default(np);

	imx9_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);

	return 0;
}

static const struct of_device_id phycore_imx93_of_match[] = {
	{
		.compatible = "phytec,imx93-phyboard-segin",
	},
	{ /* sentinel */ },
};

static struct driver phycore_imx93_board_driver = {
	.name = "board-phycore_imx93",
	.probe = phycore_imx93_probe,
	.of_compatible = phycore_imx93_of_match,
};
coredevice_platform_driver(phycore_imx93_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(phycore_imx93_of_match);
