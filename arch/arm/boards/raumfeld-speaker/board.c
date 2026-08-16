// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <mach/pxa/bbu.h>

static int raumfeld_speaker_probe(struct device *dev)
{
	barebox_set_model("Raumfeld Speaker");
	barebox_set_hostname("speaker");

	/*
	 * The Boot ROM boots from the start of NAND, so this is the partition
	 * holding the NTIM header, the OBM and barebox itself.
	 */
	pxa_bbu_nand_register_handler("nand", "/dev/nand0.barebox");

	return 0;
}

static const struct of_device_id raumfeld_speaker_of_match[] = {
	{ .compatible = "raumfeld,raumfeld-speaker-m-pxa303" },
	{ .compatible = "raumfeld,raumfeld-speaker-l-pxa303" },
	{ .compatible = "raumfeld,raumfeld-speaker-s-pxa303" },
	{ .compatible = "raumfeld,raumfeld-speaker-one-pxa303" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, raumfeld_speaker_of_match);

static struct driver raumfeld_speaker_driver = {
	.name = "board-raumfeld-speaker",
	.probe = raumfeld_speaker_probe,
	.of_compatible = raumfeld_speaker_of_match,
};
coredevice_platform_driver(raumfeld_speaker_driver);
