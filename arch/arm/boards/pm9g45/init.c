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
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <gpio.h>
#include <io.h>
#include <envfs.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <mach/io.h>
#include <mach/at91sam9_smc.h>
#include <linux/w1-gpio.h>
#include <w1_mac_address.h>

struct w1_gpio_platform_data w1_pdata = {
	.pin = AT91_PIN_PA31,
	.is_open_drain = 0,
};

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PD3,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.bus_width_16	= 0,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config pm_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 2,
	.nrd_pulse		= 3,
	.ncs_write_pulse	= 3,
	.nwe_pulse		= 4,

	.read_cycle		= 4,
	.write_cycle		= 7,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 3,
};

static void pm_add_device_nand(void)
{
	pm_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &pm_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata mci_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PD6,
	.wp_pin		= -EINVAL,
};

static void pm9g45_add_device_mci(void)
{
	at91_add_device_mci(0, &mci_data);
}
#else
static void pm9g45_add_device_mci(void) {}
#endif

/*
 * USB OHCI Host port
 */
#ifdef CONFIG_USB_OHCI_AT91
static struct at91_usbh_data  __initdata usbh_data = {
	.ports		= 2,
	.vbus_pin	= { AT91_PIN_PD0,  -EINVAL },
};

static void __init pm9g45_add_device_usbh(void)
{
	at91_add_device_usbh_ohci(&usbh_data);
}
#else
static void __init pm9g45_add_device_usbh(void) {}
#endif

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static void pm9g45_phy_init(void)
{
	/*
	 * PD2 enables the 50MHz oscillator for Ethernet PHY
	 * 1 - enable
	 * 0 - disable
	 */
	at91_set_gpio_output(AT91_PIN_PD2, 1);
	gpio_set_value(AT91_PIN_PD2, 1);
}

static void pm9g45_add_device_eth(void)
{
	w1_local_mac_address_register(0, "ron", "w1-1-0");
	pm9g45_phy_init();
	at91_add_device_eth(0, &macb_pdata);
}

static int pm9g45_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(pm9g45_mem_init);

static int pm9g45_devices_init(void)
{
	at91_set_gpio_input(w1_pdata.pin, 0);
	add_generic_device_res("w1-gpio", DEVICE_ID_SINGLE, NULL, 0, &w1_pdata);

	pm_add_device_nand();
	pm9g45_add_device_mci();
	pm9g45_add_device_eth();
	pm9g45_add_device_usbh();

	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_set_architecture(MACH_TYPE_PM9G45);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_pm9g45);

	return 0;
}
device_initcall(pm9g45_devices_init);

static int pm9g45_console_init(void)
{
	barebox_set_model("Ronetix PM9G45");
	barebox_set_hostname("pm9g45");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(pm9g45_console_init);

static int pm9g45_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(pm9g45_main_clock);
