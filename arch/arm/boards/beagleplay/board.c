// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "beagleplay: " fmt

#include <linux/kernel.h>
#include <mach/k3/common.h>
#include <driver.h>
#include <bbu.h>

static int beagleplay_probe(struct device *dev)
{
	am62x_enable_32k_crystal();

	return 0;
}

static __maybe_unused struct of_device_id beagleplay_ids[] = {
	{
		.compatible = "beagle,am625-beagleplay",
	}, {
		/* sentinel */
	}
};

static struct driver beagleplay_driver = {
	.name = "beagleplay",
	.probe = beagleplay_probe,
	.of_compatible = beagleplay_ids,
};
coredevice_platform_driver(beagleplay_driver);
