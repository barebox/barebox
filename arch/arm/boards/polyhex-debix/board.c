// SPDX-License-Identifier: GPL-2.0

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <linux/nvmem-consumer.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>
#include <net.h>

struct debix_polyhex_machine_data {
	void (*ethernet_setup)(void);
};

#define ETH_ALEN_ASCII	12

static int polyhex_debix_eth_register_ethaddr(struct device_node *np)
{
	u8 mac[ETH_ALEN];
	u8 *data;
	int ret;

	data = nvmem_cell_get_and_read(np, "mac-address", ETH_ALEN_ASCII);
	if (IS_ERR(data))
		return PTR_ERR(data);

	ret = hex2bin(mac, data, ETH_ALEN);
	if (ret)
		goto err;

	of_eth_register_ethaddr(np, mac);
err:
	free(data);

	return ret;
}

static void polyhex_debix_ethernet_init(void)
{
	static const char * const aliases[] = { "ethernet0", "ethernet1" };
	struct device_node *np, *root;
	unsigned int i;

	root = of_get_root_node();

	for (i = 0; i < ARRAY_SIZE(aliases); i++) {
		const char *alias = aliases[i];
		int ret;

		np = of_find_node_by_alias(root, alias);
		if (!np) {
			pr_warn("Failed to find %s\n", alias);
			continue;
		}

		ret = polyhex_debix_eth_register_ethaddr(np);
		if (ret) {
			pr_warn("Failed to register MAC for %s\n", alias);
			continue;
		}
	}
}

static int polyhex_debix_probe(struct device *dev)
{
	const struct debix_polyhex_machine_data *machine_data;
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

	machine_data = device_get_match_data(dev);
	if (machine_data && machine_data->ethernet_setup)
		machine_data->ethernet_setup();

	return 0;
}

static const struct debix_polyhex_machine_data debix_som_a_bmb_08 = {
	.ethernet_setup = polyhex_debix_ethernet_init,
};

static const struct of_device_id polyhex_debix_of_match[] = {
	{ .compatible = "polyhex,imx8mp-debix" },
	{ .compatible = "polyhex,imx8mp-debix-som-a-bmb-08", .data = &debix_som_a_bmb_08 },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(polyhex_debix_of_match);

static struct driver polyhex_debix_board_driver = {
	.name = "board-imx8mp-debix",
	.probe = polyhex_debix_probe,
	.of_compatible = polyhex_debix_of_match,
};
coredevice_platform_driver(polyhex_debix_board_driver);
