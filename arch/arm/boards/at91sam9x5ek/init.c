/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <fec.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <sizes.h>
#include <linux/mtd/nand.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <mach/sam9_smc.h>
#include <gpio.h>
#include <mach/io.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9x5_matrix.h>
#include <gpio_keys.h>
#include <readkey.h>
#include <linux/w1-gpio.h>

#include "hw_version.h"

struct w1_gpio_platform_data w1_pdata = {
	.pin = AT91_PIN_PB18,
	.is_open_drain = 0,
};

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= 0,
	.rdy_pin	= AT91_PIN_PD5,
	.enable_pin	= AT91_PIN_PD4,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#endif
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config cm_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 6,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 5,
	.nwe_pulse		= 3,

	.read_cycle		= 6,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 1,
};

static void ek_add_device_nand(void)
{
	/* setup bus-width (8 or 16) */
	if (nand_pdata.bus_width_16)
		cm_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		cm_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(3, &cm_nand_smc_config);

	if (at91sam9x5ek_cm_is_vendor(VENDOR_COGENT)) {
		unsigned long csa;

		csa = at91_sys_read(AT91_MATRIX_EBICSA);
		csa |= AT91_MATRIX_EBI_VDDIOMSEL_1_8V;
		at91_sys_write(AT91_MATRIX_EBICSA, csa);
	}

	at91_add_device_nand(&nand_pdata);
}

static struct at91_ether_platform_data macb_pdata = {
	.flags    = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

/*
 * USB Host port
 */
static struct at91_usbh_data __initdata ek_usbh_data = {
	.ports		= 2,
	.vbus_pin	= {AT91_PIN_PD20, AT91_PIN_PD19},
};

struct gpio_led leds[] = {
	{
		.gpio	= AT91_PIN_PB18,
		.active_low	= 1,
		.led	= {
			.name = "d1",
		},
	}, {
		.gpio	= AT91_PIN_PD21,
		.led	= {
			.name = "d2",
		},
	},
};

static void __init ek_add_led(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		at91_set_gpio_output(leds[i].gpio, leds[i].active_low);
		led_gpio_register(&leds[i]);
	}
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &leds[0].led);
}

static int at91sam9x5ek_mem_init(void)
{
	at91_add_device_sdram(128 * 1024 * 1024);

	return 0;
}
mem_initcall(at91sam9x5ek_mem_init);

static void ek_add_device_w1(void)
{
	at91_set_gpio_input(w1_pdata.pin, 0);
	at91_set_multi_drive(w1_pdata.pin, 1);
	add_generic_device_res("w1-gpio", DEVICE_ID_SINGLE, NULL, 0, &w1_pdata);

	at91sam9x5ek_devices_detect_hw();
}

static int at91sam9x5ek_devices_init(void)
{
	ek_add_device_w1();
	ek_add_device_nand();
	at91_add_device_eth(0, &macb_pdata);
	at91_add_device_usbh_ohci(&ek_usbh_data);
	ek_add_led();

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(CONFIG_MACH_AT91SAM9X5EK);

	devfs_add_partition("nand0", 0x00000, SZ_256K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_256K, SZ_256K + SZ_128K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_512K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K + SZ_256K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	return 0;
}
device_initcall(at91sam9x5ek_devices_init);

static int at91sam9x5ek_console_init(void)
{
	at91_register_uart(0, 0);
	at91_register_uart(1, 0);
	return 0;
}
console_initcall(at91sam9x5ek_console_init);
