// SPDX-License-Identifier: GPL-2.0-only
/*
 * Board code for the WolfVision PF5 mainboard.
 *
 * Copyright (C) 2024 WolfVision GmbH.
 */

#define pr_fmt(fmt) "WolfVision PF5: " fmt

#include <common.h>
#include <deep-probe.h>
#include <globalvar.h>
#include <init.h>

#include <boards/wolfvision/common.h>
#include <mach/rockchip/bbu.h>

#define PF5_DISPLAY_VZ_FILENAME "rk3568-wolfvision-pf5-display-vz.dtbo"
#define PF5_DISPLAY_VZ_DATA __dtbo_rk3568_wolfvision_pf5_display_vz_start
#define PF5_IO_EXPANDER_FILENAME "rk3568-wolfvision-pf5-io-expander.dtbo"
#define PF5_IO_EXPANDER_DATA __dtbo_rk3568_wolfvision_pf5_io_expander_start

enum {
	PF5_HWID_CHANNEL_MAINBOARD = 1,
	PF5_HWID_CHANNEL_MODULE = 2,
	PF5_HWID_CHANNEL_DISPLAY = 3,
};

extern char PF5_IO_EXPANDER_DATA[];
extern char PF5_DISPLAY_VZ_DATA[];

static const struct wv_rk3568_extension pf5_extensions[] = {
	{
		.adc_chan = PF5_HWID_CHANNEL_MAINBOARD,
		.name = "mainboard",
		.overlays = {
			[0] = { .name = "PF5 DC V1.0 A", },
			[4] = { .name = "PF5 DC V1.1 A", },
		},
	},
	{
		.adc_chan = PF5_HWID_CHANNEL_MODULE,
		.name = "module",
		.overlays = {
			[0] = { .name = "PF5 IO Expander V1.0 A",
				.filename = PF5_IO_EXPANDER_FILENAME,
				.data = PF5_IO_EXPANDER_DATA,
			},
			[16] = { .name = "no", },
		},
	},
	{
		.adc_chan = PF5_HWID_CHANNEL_DISPLAY,
		.name = "display",
		.overlays = {
			[0] = { .name = "Visualizer",
				.filename = PF5_DISPLAY_VZ_FILENAME,
				.data = PF5_DISPLAY_VZ_DATA,
			},
			[16] = { .name = "no" },
		},
	},
};

static int pf5_probe(struct device *dev)
{
	char *pf5_overlays = NULL;
	int ret;

	barebox_set_model("WolfVision PF5");
	barebox_set_hostname("PF5");

	ret = wolfvision_register_ethaddr();
	if (ret)
		pr_warning("failed to register MAC addresses\n");

	rk3568_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");

	ret = wolfvision_rk3568_detect_hw(
		pf5_extensions, ARRAY_SIZE(pf5_extensions), &pf5_overlays);
	if (ret)
		pr_warning("failed to detect HW\n");

	if (pf5_overlays)
		globalvar_set("of.overlay.filepattern", pf5_overlays);

	free(pf5_overlays);

	return 0;
}

static const struct of_device_id pf5_of_match[] = {
	{
		.compatible = "wolfvision,rk3568-pf5",
	},
	{ /* sentinel */ },
};

static struct driver_d pf5_board_driver = {
	.name = "board-wolfvision-pf5",
	.probe = pf5_probe,
	.of_compatible = pf5_of_match,
};
coredevice_platform_driver(pf5_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(pf5_of_match);
