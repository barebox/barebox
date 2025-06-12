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

static int tac_probe(struct device *dev)
{
	struct tlv_device *tlvcpu;

	barebox_set_hostname("lxatac");

	stm32mp_bbu_mmc_fip_register("mmc", "/dev/mmc1", BBU_HANDLER_FLAG_DEFAULT);

	tlvcpu = tlv_ensure_probed_by_alias("baseboard-factory-data");
	if (!IS_ERR(tlvcpu))
		dev_info(dev, "HW release: %s\n",
			 dev_get_param(&tlvcpu->dev, "device-hardware-release"));

	return 0;
}

static const struct of_device_id tac_of_match[] = {
	{ .compatible = "lxa,stm32mp157c-tac-gen1" },
	{ .compatible = "lxa,stm32mp157c-tac-gen2" },
	{ .compatible = "lxa,stm32mp153c-tac-gen3" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(tac_of_match);

static struct driver tac_board_driver = {
	.name = "board-lxa-tac",
	.probe = tac_probe,
	.of_compatible = tac_of_match,
};
late_platform_driver(tac_board_driver);
