// SPDX-License-Identifier: GPL-2.0-only

#include <deep-probe.h>
#include <bootsource.h>
#include <driver.h>
#include <init.h>
#include <bbu.h>
#include <of.h>

static int wifx_l1_probe(struct device *dev)
{
	int flags_sd = 0;

	if (bootsource_get() == BOOTSOURCE_NAND) {
		of_device_enable_path("/chosen/environment-nand");
	} else {
		of_device_enable_path("/chosen/environment-microsd");
		flags_sd = BBU_HANDLER_FLAG_DEFAULT;
	}

	bbu_register_std_file_update("sd", flags_sd, "/mnt/mmc1.0/barebox.bin",
				     filetype_arm_barebox);

	return 0;
}

static const struct of_device_id wifx_l1_of_match[] = {
	{ .compatible = "wifx,l1" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(wifx_l1_of_match);

static struct driver wifx_l1_board_driver = {
	.name = "board-lxa-mc1",
	.probe = wifx_l1_probe,
	.of_compatible = wifx_l1_of_match,
};
device_platform_driver(wifx_l1_board_driver);
