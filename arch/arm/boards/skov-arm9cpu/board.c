// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2017 Sam Ravnborg <sam@ravnborg.org>

#include <common.h>
#include <globalvar.h>
#include <magicvar.h>
#include <envfs.h>
#include <init.h>
#include <gpio.h>

#include <linux/sizes.h>

#include <mach/at91/at91sam9263_matrix.h>
#include <mach/at91/at91sam9_sdramc.h>
#include <mach/at91/at91sam9_smc.h>
#include <mach/at91/hardware.h>
#include <mach/at91/iomux.h>

static struct sam9_smc_config skov_nor_smc_config = {
	/* Setup time is 2 cycles after the CS signal */
	.nwe_setup		= 2,
	.ncs_write_setup	= 0,
	.nrd_setup		= 2,
	.ncs_read_setup		= 0,

	/* Set pulse long enough - pulse should be a bit shorter than the cycle */
	.nwe_pulse		= 10,
	.ncs_write_pulse	= 12,
	.nrd_pulse		= 10,
	.ncs_read_pulse		= 12,

	/* Set cycle long enougth at least 12 Cycles->120ns plus a little extra */
	.write_cycle		= 0x13,
	.read_cycle		= 0x13,

	/* Set mode: 16Bit bus width, enable read and write
	 *   Note: pagemode + 32 byte pages do not work with the 29GL512P flash
	 */
	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
				  AT91_SMC_EXNWMODE_DISABLE |
				  AT91_SMC_BAT_WRITE |
				  AT91_SMC_DBW_16 |
				  AT91_SMC_TDFMODE,
	.tdf_cycles		= 1,
};

BAREBOX_MAGICVAR(board.mem, "The detected memory size in MiB");

static int mem;

/*
 * Initialize of SMC must come after we
 * probe the at91sam9_smc_driver.
 * But is required before we start the other drives.
 * Use device_initcall() to maintain this order.
 */
static int skov_arm9_probe(struct device *dev)
{
	add_generic_device("at91sam9-smc", 0, NULL, AT91SAM9263_BASE_SMC0, 0x200,
			   IORESOURCE_MEM, NULL);
	add_generic_device("at91sam9-smc", 1, NULL, AT91SAM9263_BASE_SMC1, 0x200,
			   IORESOURCE_MEM, NULL);

	/* configure chip-select 0 (NOR) */
	sam9_smc_configure(0, 0, &skov_nor_smc_config);

	mem = at91_get_sdram_size(IOMEM(AT91SAM9263_BASE_SDRAMC0));
	mem = mem / SZ_1M;
	globalvar_add_simple_int("board.mem", &mem, "%u");

	return 0;
}

static __maybe_unused struct of_device_id skov_arm9_ids[] = {
	{
		.compatible = "skov,arm9-cpu",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, skov_arm9_ids);

static struct driver skov_arm9_driver = {
	.name = "skov-arm9",
	.probe = skov_arm9_probe,
	.of_compatible = DRV_OF_COMPAT(skov_arm9_ids),
};
coredevice_platform_driver(skov_arm9_driver);
