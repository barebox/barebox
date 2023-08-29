// SPDX-License-Identifier: GPL-2.0

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>

static int polyhex_debix_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;
	u32 val;

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 1) {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	/* Enable RGMII TX clk output */
	val = readl(MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);
	val |= MX8MP_IOMUXC_GPR1_ENET1_RGMII_EN |
	       MX8MP_IOMUXC_GPR1_ENET_QOS_RGMII_EN;
	writel(val, MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);

	return 0;
}

static const struct of_device_id polyhex_debix_of_match[] = {
	{ .compatible = "polyhex,imx8mp-debix" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(polyhex_debix_of_match);

static struct driver polyhex_debix_board_driver = {
	.name = "board-imx8mp-debix",
	.probe = polyhex_debix_probe,
	.of_compatible = polyhex_debix_of_match,
};
coredevice_platform_driver(polyhex_debix_board_driver);
