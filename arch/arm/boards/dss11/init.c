/*
 * Copyright (C) 2011 Michael Grzeschik <mgr@pengutronix.de>
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
#include <mci.h>
#include <init.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/at91_rstc.h>
#include <linux/clk.h>

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.ecc_mode	= NAND_ECC_HW,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.bus_width_16	= 1,
};

static struct sam9_smc_config dss11_nand_smc_config = {
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

static void dss11_add_device_nand(void)
{
	/* setup bus-width (16) */
	dss11_nand_smc_config.mode |= AT91_SMC_DBW_16;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &dss11_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct macb_platform_data macb_pdata = {
	.phy_addr = 0,
	.flags = AT91SAM_ETX2_ETX3_ALTERNATIVE,
};

static void dss11_phy_reset(void)
{
	at91_set_gpio_input(AT91_PIN_PA14, 0);
	at91_set_gpio_input(AT91_PIN_PA15, 0);
	at91_set_gpio_input(AT91_PIN_PA17, 0);
	at91_set_gpio_input(AT91_PIN_PA25, 0);
	at91_set_gpio_input(AT91_PIN_PA26, 0);
	at91_set_gpio_input(AT91_PIN_PA28, 0);

	at91sam_phy_reset(IOMEM(AT91SAM9260_BASE_RSTC));
}

static struct atmel_mci_platform_data dss11_mci_data = {
	.slot_b		= 1,
	.bus_width	= 4,
	.detect_pin     = -EINVAL,
	.wp_pin		= -EINVAL,
};

static struct at91_usbh_data dss11_usbh_data = {
	.ports		= 2,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

static int dss11_mem_init(void)
{
	at91_add_device_sdram(64 * 1024 * 1024);

	return 0;
}
mem_initcall(dss11_mem_init);

static int dss11_devices_init(void)
{
	dss11_add_device_nand();
	dss11_phy_reset();
	at91_add_device_eth(0, &macb_pdata);
	at91_add_device_mci(0, &dss11_mci_data);
	at91_add_device_usbh_ohci(&dss11_usbh_data);

	armlinux_set_architecture(MACH_TYPE_DSS11);

	devfs_add_partition("nand0", 0x00000, 0x20000, DEVFS_PARTITION_FIXED, "bootstrap");
	dev_add_bb_dev("bootstrap", "bootstrap.bb");
	devfs_add_partition("nand0", 0x20000, 0x40000, DEVFS_PARTITION_FIXED, "barebox");
	dev_add_bb_dev("barebox", "barebox.bb");
	devfs_add_partition("nand0", 0x60000, 0x40000, DEVFS_PARTITION_FIXED, "barebox-env");
	dev_add_bb_dev("barebox-env", "env0");

	return 0;
}
device_initcall(dss11_devices_init);

static int dss11_console_init(void)
{
	barebox_set_model("Aizo dSS11");
	barebox_set_hostname("dss11");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(dss11_console_init);

static int dss11_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(dss11_main_clock);
