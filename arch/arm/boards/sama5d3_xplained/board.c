// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <envfs.h>
#include <mach/at91/at91sam9_smc.h>
#include <mach/at91/hardware.h>
#include <linux/clk.h>

static struct sam9_smc_config sama5d3_xplained_nand_smc_config = {
	.ncs_read_setup		= 1,
	.nrd_setup		= 2,
	.ncs_write_setup	= 1,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 5,
	.nrd_pulse		= 3,
	.ncs_write_pulse	= 5,
	.nwe_pulse		= 3,

	.read_cycle		= 8,
	.write_cycle		= 8,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
				  AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 3,

	.tclr			= 3,
	.tadl			= 10,
	.tar			= 3,
	.ocms			= 0,
	.trr			= 4,
	.twb			= 5,
	.rbnsel			= 3,
	.nfsel			= 1
};

static int sama5d3_xplained_probe(struct device *dev)
{
	struct clk *clk;

	barebox_set_hostname("sama5d3_xplained");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_sama5d3_xplained);

	add_generic_device("at91sam9-smc", DEVICE_ID_SINGLE, NULL,
			   SAMA5D3_BASE_HSMC + 0x600, 0xa0,
			   IORESOURCE_MEM, NULL);

	clk = clk_lookup("hsmc_clk");
	if (IS_ERR(clk))
		dev_warn(dev, "couldn't get hsmc_clk: %pe\n", clk);

	clk_enable(clk);

	/* configure chip-select 3 (NAND) */
	sama5_smc_configure(0, 3, &sama5d3_xplained_nand_smc_config);

	return 0;
}

static const struct of_device_id sama5d3_xplained_of_match[] = {
	{ .compatible = "atmel,sama5d3-xplained" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sama5d3_xplained_of_match);

static struct driver sama5d3_xplained_board_driver = {
	.name = "board-sama5d3_xplained",
	.probe = sama5d3_xplained_probe,
	.of_compatible = sama5d3_xplained_of_match,
};
coredevice_platform_driver(sama5d3_xplained_board_driver);
