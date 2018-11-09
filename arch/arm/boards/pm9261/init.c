/*
 * Copyright (C) 2009-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
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
 *
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <gpio.h>
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
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <mach/at91sam9_smc.h>
#include <platform_data/eth-dm9000.h>
#include <linux/w1-gpio.h>
#include <w1_mac_address.h>

struct w1_gpio_platform_data w1_pdata = {
	.pin = AT91_PIN_PA7,
	.is_open_drain = 0,
};

static struct atmel_nand_data nand_pdata = {
	.ale		= 22,
	.cle		= 21,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PA16,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
};

static struct sam9_smc_config pm_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 2,
};

static void pm_add_device_nand(void)
{
	/* setup bus-width (8 or 16) */
	if (nand_pdata.bus_width_16)
		pm_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		pm_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &pm_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

/*
 * DM9000 ethernet device
 */
#if defined(CONFIG_DRIVER_NET_DM9K)
static struct dm9000_platform_data dm9000_data = {
	.srom		= 1,
};

/*
 * SMC timings for the DM9000.
 * Note: These timings were calculated for MASTER_CLOCK = 100000000 according to the DM9000 timings.
 */
static struct sam9_smc_config __initdata dm9000_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 8,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 8,
	.nwe_pulse		= 4,

	.read_cycle		= 16,
	.write_cycle		= 16,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_BAT_WRITE | AT91_SMC_DBW_16,
	.tdf_cycles		= 1,
};

static void __init pm_add_device_dm9000(void)
{
	w1_local_mac_address_register(0, "ron", "w1-1-0");
	/* Configure chip-select 2 (DM9000) */
	sam9_smc_configure(0, 2, &dm9000_smc_config);

	add_dm9000_device(0, AT91_CHIPSELECT_2, AT91_CHIPSELECT_2 + 4,
			  IORESOURCE_MEM_16BIT, &dm9000_data);
}
#else
static void __init pm_add_device_dm9000(void) {}
#endif /* CONFIG_DRIVER_NET_DM9K */

static int pm9261_mem_init(void)
{
	at91_add_device_sdram(64 * 1024 * 1024);

	return 0;
}
mem_initcall(pm9261_mem_init);

static int pm9261_devices_init(void)
{
	pm_add_device_nand();
	pm_add_device_dm9000();
	add_cfi_flash_device(0, AT91_CHIPSELECT_0, 4 * 1024 * 1024, 0);

	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 0x10000, DEVFS_PARTITION_FIXED, "env0");

	armlinux_set_architecture(MACH_TYPE_PM9261);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_pm9261);

	return 0;
}
device_initcall(pm9261_devices_init);

static int pm9261_console_init(void)
{
	barebox_set_model("Ronetix PM9261");
	barebox_set_hostname("pm9261");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(pm9261_console_init);

static int pm9261_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(pm9261_main_clock);
