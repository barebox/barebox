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

static struct sam9_smc_config ek_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 3,
	.nrd_pulse		= 3,
	.ncs_write_pulse	= 3,
	.nwe_pulse		= 3,

	.read_cycle		= 5,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
				  AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 2,
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
	unsigned long csa;

	add_generic_device("at91sam9-smc", 0, NULL, AT91SAM9263_BASE_SMC0, 0x200,
			   IORESOURCE_MEM, NULL);
	add_generic_device("at91sam9-smc", 1, NULL, AT91SAM9263_BASE_SMC1, 0x200,
			   IORESOURCE_MEM, NULL);

	csa = readl(AT91SAM9263_BASE_MATRIX + AT91SAM9263_MATRIX_EBI0CSA);
	csa |= AT91SAM9263_MATRIX_EBI0_CS3A_SMC_SMARTMEDIA;
	writel(csa, AT91SAM9263_BASE_MATRIX + AT91SAM9263_MATRIX_EBI0CSA);

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

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
device_platform_driver(skov_arm9_driver);
