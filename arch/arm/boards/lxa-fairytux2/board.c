// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/stm32mp/bbu.h>
#include <bootsource.h>
#include <deep-probe.h>
#include <of.h>
#include <tlv/tlv.h>

static int fairytux2_probe(struct device *dev)
{
	struct tlv_device *tlvcpu;

	barebox_set_hostname("fairytux2");

	stm32mp_bbu_mmc_fip_register("mmc", "/dev/mmc1", BBU_HANDLER_FLAG_DEFAULT);

	tlvcpu = tlv_ensure_probed_by_alias("baseboard-factory-data");
	if (!IS_ERR(tlvcpu))
		dev_info(dev, "HW release: %s\n",
			 dev_get_param(&tlvcpu->dev, "device-hardware-release"));

	return 0;
}

static const struct of_device_id fairytux2_of_match[] = {
	{ .compatible = "lxa,stm32mp153c-fairytux2-gen1" },
	{ .compatible = "lxa,stm32mp153c-fairytux2-gen2" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(fairytux2_of_match);

static struct driver fairytux2_board_driver = {
	.name = "board-lxa-fairytux2",
	.probe = fairytux2_probe,
	.of_compatible = fairytux2_of_match,
};
device_platform_driver(fairytux2_board_driver);
