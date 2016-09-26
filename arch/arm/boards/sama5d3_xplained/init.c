/*
 * Copyright (C) 2014 Bo Shen <voice.shen@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <envfs.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <gpio.h>
#include <mach/io.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9x5_matrix.h>
#include <readkey.h>
#include <poller.h>
#include <linux/clk.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>

#if defined(CONFIG_NAND_ATMEL)
static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= -EINVAL,
	.enable_pin	= -EINVAL,
	.ecc_mode	= NAND_ECC_HW,
	.has_pmecc	= 1,
	.pmecc_sector_size = 512,
	.pmecc_corr_cap = 4,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#endif
	.on_flash_bbt	= 1,
};

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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
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

static void ek_add_device_nand(void)
{
	struct clk *clk = clk_get(NULL, "smc_clk");

	clk_enable(clk);

	/* setup bus-width (8 or 16) */
	if (nand_pdata.bus_width_16)
		sama5d3_xplained_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		sama5d3_xplained_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sama5_smc_configure(0, 3, &sama5d3_xplained_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}
#else
static void ek_add_device_nand(void) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
static struct macb_platform_data gmac_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RGMII,
	.phy_addr = 7,
};

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static void ek_add_device_eth(void)
{
	at91_add_device_eth(0, &gmac_pdata);
	at91_add_device_eth(1, &macb_pdata);
}
#else
static void ek_add_device_eth(void) {}
#endif

#if defined(CONFIG_MCI_ATMEL)
/*
 * MCI (SD/MMC)
 */
static struct atmel_mci_platform_data mci0_data = {
	.bus_width	= 8,
	.detect_pin	= AT91_PIN_PE0,
	.wp_pin		= -EINVAL,
};

static void ek_add_device_mci(void)
{
	/* MMC0 */
	at91_add_device_mci(0, &mci0_data);
}
#else
static void ek_add_device_mci(void) {}
#endif

#ifdef CONFIG_LED_GPIO
struct gpio_led leds[] = {
	{
		.gpio	= AT91_PIN_PE23,
		.active_low	= 1,
		.led	= {
			.name = "d2",
		},
	}, {
		.gpio	= AT91_PIN_PE24,
		.active_low	= 1,
		.led	= {
			.name = "d3",
		},
	},
};

static void ek_add_led(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		at91_set_gpio_output(leds[i].gpio, leds[i].active_low);
		led_gpio_register(&leds[i]);
	}
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &leds[0].led);
}
#else
static void ek_add_led(void) {}
#endif

static int sama5d3_xplained_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(sama5d3_xplained_mem_init);

static const struct devfs_partition sama5d3_xplained_nand0_partitions[] = {
	{
		.offset = 0x00000,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "at91bootstrap_raw",
		.bbname = "at91bootstrap",
	}, {
		.offset = DEVFS_PARTITION_APPEND, /* 256 KiB */
		.size = SZ_256K + SZ_128K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "self_raw",
		.bbname = "self0",
	},
	/* hole of 128 KiB */
	{
		.offset = SZ_512K + SZ_256K,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw",
		.bbname = "env0",
	}, {
		.offset = DEVFS_PARTITION_APPEND, /* 1 MiB */
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw1",
		.bbname = "env1",
	}, {
		/* sentinel */
	}
};

static int sama5d3_xplained_devices_init(void)
{
	ek_add_device_nand();
	ek_add_led();
	ek_add_device_eth();
	ek_add_device_mci();

	devfs_create_partitions("nand0", sama5d3_xplained_nand0_partitions);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_sama5d3_xplained);

	return 0;
}
device_initcall(sama5d3_xplained_devices_init);

static int sama5d3_xplained_console_init(void)
{
	barebox_set_model("Atmel sama5d3_xplained");
	barebox_set_hostname("sama5d3_xplained");

	at91_register_uart(0, 0);

	return 0;
}
console_initcall(sama5d3_xplained_console_init);

static int sama5d3_xplained_main_clock(void)
{
	at91_set_main_clock(12000000);

	return 0;
}
pure_initcall(sama5d3_xplained_main_clock);
