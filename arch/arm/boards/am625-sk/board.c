// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "am625-sk: " fmt

#include <linux/kernel.h>
#include <mach/k3/common.h>
#include <driver.h>
#include <bbu.h>

static int am625_sk_probe(struct device *dev)
{
	am62x_enable_32k_crystal();

	k3_bbu_emmc_register("emmc", "/dev/mmc0", BBU_HANDLER_FLAG_DEFAULT);

	if (k3_boot_is_emmc())
		of_device_enable_path("/chosen/environment-emmc");

	return 0;
}

static __maybe_unused struct of_device_id am625_sk_ids[] = {
	{
		.compatible = "ti,am625-sk",
	}, {
		/* sentinel */
	}
};

static struct driver am625_sk_driver = {
	.name = "am625-sk",
	.probe = am625_sk_probe,
	.of_compatible = am625_sk_ids,
};
coredevice_platform_driver(am625_sk_driver);
