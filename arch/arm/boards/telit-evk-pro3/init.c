/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2013 Fabio Porcedda <fabio.porcedda@gmail.com>, Telit
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

#include <asm/armlinux.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/mtd/nand.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9_smc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <nand.h>

#define BOOTSTRAP_SIZE	0xC0000

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config evk_nand_smc_config = {
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
				  AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 2,
};

static void evk_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &evk_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static void evk_phy_reset(void)
{
	at91_set_gpio_input(AT91_PIN_PA14, 0);
	at91_set_gpio_input(AT91_PIN_PA15, 0);
	at91_set_gpio_input(AT91_PIN_PA17, 0);
	at91_set_gpio_input(AT91_PIN_PA25, 0);
	at91_set_gpio_input(AT91_PIN_PA26, 0);
	at91_set_gpio_input(AT91_PIN_PA28, 0);

	at91sam_phy_reset(IOMEM(AT91SAM9260_BASE_RSTC));
}

/*
 * MCI (SD/MMC)
 */
#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata evk_mci_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PA27,
	.wp_pin		= AT91_PIN_PA25,
};

static void evk_usb_add_device_mci(void)
{
	at91_add_device_mci(0, &evk_mci_data);
}
#else
static void evk_usb_add_device_mci(void) {}
#endif

/*
 * USB Host port
 */
static struct at91_usbh_data __initdata evk_usbh_data = {
	.ports		= 2,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

/*
 * USB Device port
 */
static struct at91_udc_data __initdata evk_udc_data = {
	.vbus_pin	= AT91_PIN_PC4,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

static int evk_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(evk_mem_init);

static int evk_devices_init(void)
{
	evk_add_device_nand();
	evk_phy_reset();
	at91_add_device_eth(0, &macb_pdata);
	at91_add_device_usbh_ohci(&evk_usbh_data);
	at91_add_device_udc(&evk_udc_data);
	evk_usb_add_device_mci();

	devfs_add_partition("nand0", 0x00000, BOOTSTRAP_SIZE,
			    DEVFS_PARTITION_FIXED, "bootstrap_raw");
	dev_add_bb_dev("bootstrap_raw", "bootstrap");
	devfs_add_partition("nand0", BOOTSTRAP_SIZE, SZ_256K,
			    DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", BOOTSTRAP_SIZE + SZ_256K, SZ_128K,
			    DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", BOOTSTRAP_SIZE + SZ_256K + SZ_128K,
			    SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	return 0;
}
device_initcall(evk_devices_init);

static int evk_console_init(void)
{
	barebox_set_model("Telit EVK-PRO3");
	barebox_set_hostname("evkpr03");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(evk_console_init);

static int evk_main_clock(void)
{
	at91_set_main_clock(6000000);
	return 0;
}
pure_initcall(evk_main_clock);
