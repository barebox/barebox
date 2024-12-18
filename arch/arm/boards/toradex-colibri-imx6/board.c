// SPDX-License-Identifier: GPL-2.0-or-later

#include <asm/armlinux.h>
#include <common.h>
#include <environment.h>
#include <mach/imx/bbu.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx6.h>
#include <mfd/imx6q-iomuxc-gpr.h>

static void eth_init(void)
{
	void __iomem *iomux = IOMEM(MX6_IOMUXC_BASE_ADDR);
	uint32_t val;

	/* get enet tx reference clk from internal clock */
	val = readl(iomux + IOMUXC_GPR1);
	val |= IMX6Q_GPR1_ENET_CLK_SEL_ANATOP;
	writel(val, iomux + IOMUXC_GPR1);
}

static int colibri_imx6_probe(struct device *dev)
{
	imx6_bbu_internal_mmcboot_register_handler("emmc", "/dev/mmc0",
			BBU_HANDLER_FLAG_DEFAULT);
	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc1", 0);

	barebox_set_hostname("colibri-imx6");

	eth_init();

	return 0;
}

static const struct of_device_id colibri_imx6_match[] = {
	{ .compatible = "toradex,colibri_imx6dl"},
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(colibri_imx6_match);

static struct driver colibri_imx6_board_driver = {
	.name = "board-colibri-imx6",
	.probe = colibri_imx6_probe,
	.of_compatible = DRV_OF_COMPAT(colibri_imx6_match),
};
device_platform_driver(colibri_imx6_board_driver);
