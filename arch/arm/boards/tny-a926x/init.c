/*
 * Copyright (C) 2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <io.h>
#include <envfs.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <linux/clk.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91sam9_sdramc.h>
#include <gpio.h>
#include <mach/io.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <spi/eeprom.h>

static void tny_a9260_set_board_type(void)
{
	if (machine_is_tny_a9g20())
		armlinux_set_architecture(MACH_TYPE_TNY_A9G20);
	else if (machine_is_tny_a9263())
		armlinux_set_architecture(MACH_TYPE_TNY_A9263);
	else
		armlinux_set_architecture(MACH_TYPE_TNY_A9260);
}

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config tny_a9260_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | \
				  AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 2,
};

static struct sam9_smc_config tny_a9g20_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 4,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 4,
	.nwe_pulse		= 4,

	.read_cycle		= 7,
	.write_cycle		= 7,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | \
				  AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 3,
};

static void tny_a9260_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	if (machine_is_tny_a9g20())
		sam9_smc_configure(0, 3, &tny_a9g20_nand_smc_config);
	else
		sam9_smc_configure(0, 3, &tny_a9260_nand_smc_config);

	if (machine_is_tny_a9263()) {
		nand_pdata.rdy_pin	= AT91_PIN_PA22;
		nand_pdata.enable_pin	= AT91_PIN_PD15;
	}

	at91_add_device_nand(&nand_pdata);
}

#ifdef CONFIG_DRIVER_NET_MACB
static struct macb_platform_data macb_pdata = {
	.phy_interface	= PHY_INTERFACE_MODE_RMII,
	.phy_addr	= -1,
};

static void __init ek_add_device_macb(void)
{
	if (IS_ENABLED(CONFIG_CALAO_MOB_TNY_MD2))
		at91_add_device_eth(0, &macb_pdata);
}
#else
static void __init ek_add_device_macb(void) {}
#endif

/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PB30,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

static struct spi_eeprom eeprom = {
	.name = "at250x0",
	.size = 8192,
	.page_size = 32,
	.flags = EE_ADDR2,	/* 16 bits address */
};

static struct spi_board_info tny_a9g20_spi_devices[] = {
	{
		.name = "at25x",
		.max_speed_hz = 1 * 1000 * 1000,	/* max spi clock (SCK) speed in HZ */
		.bus_num = 0,
		.chip_select = 0,
		.platform_data = &eeprom,
	},
};

static struct spi_board_info tny_a9g20_lpw_spi_devices[] = {
	{
		.name = "spi_mci",
		.max_speed_hz = 25 * 1000 * 1000,
		.bus_num = 1,
		.chip_select = 0,
	},
};

static struct spi_board_info tny_a9263_spi_devices[] = {
	{
		.name = "mtd_dataflash",
		.max_speed_hz = 15 * 1000 * 1000,
		.bus_num = 0,
		.chip_select = 0,
	},
};

static int tny_a9263_spi0_standard_cs[] = { AT91_PIN_PA5 };
struct at91_spi_platform_data tny_a9263_spi0_pdata = {
	.chipselect = tny_a9263_spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(tny_a9263_spi0_standard_cs),
};

static int tny_a9g20_spi0_standard_cs[] = { AT91_PIN_PC11 };
struct at91_spi_platform_data tny_a9g20_spi0_pdata = {
	.chipselect = tny_a9g20_spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(tny_a9g20_spi0_standard_cs),
};

static int tny_a9g20_spi1_standard_cs[] = { AT91_PIN_PC3 };
struct at91_spi_platform_data tny_a9g20_spi1_pdata = {
	.chipselect = tny_a9g20_spi1_standard_cs,
	.num_chipselect = ARRAY_SIZE(tny_a9g20_spi1_standard_cs),
};

static void __init ek_add_device_udc(void)
{
	if (machine_is_tny_a9260() || machine_is_tny_a9g20())
		ek_udc_data.vbus_pin = AT91_PIN_PC5;

	at91_add_device_udc(&ek_udc_data);
}

static void __init ek_add_device_spi(void)
{
	if (machine_is_tny_a9263()) {
		spi_register_board_info(tny_a9263_spi_devices,
			ARRAY_SIZE(tny_a9263_spi_devices));
		at91_add_device_spi(0, &tny_a9263_spi0_pdata);

	} else if (machine_is_tny_a9g20() && at91sam9260_is_low_power_sdram()) {
		spi_register_board_info(tny_a9g20_lpw_spi_devices,
			ARRAY_SIZE(tny_a9g20_lpw_spi_devices));
		at91_add_device_spi(1, &tny_a9g20_spi1_pdata);
	} else {
		spi_register_board_info(tny_a9g20_spi_devices,
			ARRAY_SIZE(tny_a9g20_spi_devices));
		at91_add_device_spi(0, &tny_a9g20_spi0_pdata);
	}
}

static int tny_a9260_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(tny_a9260_mem_init);

static int tny_a9260_devices_init(void)
{
	tny_a9260_add_device_nand();
	ek_add_device_macb();
	ek_add_device_udc();
	ek_add_device_spi();

	tny_a9260_set_board_type();

	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_tny_a926x);

	return 0;
}
device_initcall(tny_a9260_devices_init);

static int tny_a9260_console_init(void)
{
	if (machine_is_tny_a9g20()) {
		barebox_set_model("Calao TNY-A9G20");
		barebox_set_hostname("tny-a9g20");
	} else if (machine_is_tny_a9263()) {
		barebox_set_model("Calao TNY-A9263");
		barebox_set_hostname("tny-a9263");
	} else {
		barebox_set_model("Calao TNY-A9260");
		barebox_set_hostname("tny-a9260");
	}

	at91_register_uart(0, 0);
	if (IS_ENABLED(CONFIG_CALAO_MOB_TNY_MD2))
		at91_register_uart(2, ATMEL_UART_CTS | ATMEL_UART_RTS);
	return 0;
}
console_initcall(tny_a9260_console_init);

static int tny_a9260_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(tny_a9260_main_clock);
