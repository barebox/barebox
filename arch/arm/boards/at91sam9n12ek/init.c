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
#include <gpio.h>
#include <mach/io.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9x5_matrix.h>
#include <input/qt1070.h>
#include <readkey.h>
#include <spi/spi.h>

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PD5,
	.enable_pin	= AT91_PIN_PD4,
	.ecc_mode	= NAND_ECC_HW,
	.pmecc_sector_size = 512,
	.pmecc_corr_cap = 2,
	.bus_on_d0	= 1,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config ek_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 6,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 5,
	.nwe_pulse		= 3,

	.read_cycle		= 7,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 1,
};

static void ek_add_device_nand(void)
{
	ek_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

/*
 * KS8851 ethernet device
 */
#if defined(CONFIG_DRIVER_NET_KS8851_MLL)
/*
 * SMC timings for the KS8851.
 * Note: These timings were calculated
 * for MASTER_CLOCK = 100000000 according to the KS8851 timings.
 */
static struct sam9_smc_config __initdata ks8851_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 7,
	.nrd_pulse		= 7,
	.ncs_write_pulse	= 7,
	.nwe_pulse		= 7,

	.read_cycle		= 9,
	.write_cycle		= 9,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_BAT_SELECT | AT91_SMC_DBW_16,
	.tdf_cycles		= 1,
};

static void __init ek_add_device_ks8851(void)
{
	/* Configure chip-select 2 (KS8851) */
	sam9_smc_configure(0, 2, &ks8851_smc_config);
	/* Configure NCS signal */
	at91_set_B_periph(AT91_PIN_PD19, 0);
	/* Configure Interrupt pin as input, no pull-up */
	at91_set_gpio_input(AT91_PIN_PD21, 0);

	add_ks8851_device(DEVICE_ID_SINGLE, AT91_CHIPSELECT_2, AT91_CHIPSELECT_2 + 2,
				IORESOURCE_MEM_16BIT, NULL);
}
#else
static void __init ek_add_device_ks8851(void) {}
#endif /* CONFIG_DRIVER_NET_KS8851_MLL */

/*
 * MCI (SD/MMC)
 */
/* mci0 detect_pin is revision dependent */
static struct atmel_mci_platform_data mci0_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PA7,
	.wp_pin		= 0,
};

static void ek_add_device_mci(void)
{
	/* MMC0 */
	at91_add_device_mci(0, &mci0_data);
}

struct qt1070_platform_data qt1070_pdata = {
	.irq_pin	= AT91_PIN_PA2,
};

static struct i2c_board_info i2c_devices[] = {
	{
		.platform_data = &qt1070_pdata,
		I2C_BOARD_INFO("qt1070", 0x1b),
	},
};

static void ek_add_device_i2c(void)
{
	at91_set_gpio_input(qt1070_pdata.irq_pin, 0);
	at91_set_deglitch(qt1070_pdata.irq_pin, 1);
	at91_add_device_i2c(0, i2c_devices, ARRAY_SIZE(i2c_devices));
}

static const struct spi_board_info ek_spi_devices[] = {
	{
		.name		= "m25p80",
		.chip_select	= 0,
		.max_speed_hz	= 1 * 1000 * 1000,
		.bus_num	= 0,
	},
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PA14 };
static struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void ek_add_device_spi(void)
{
	spi_register_board_info(ek_spi_devices,
				ARRAY_SIZE(ek_spi_devices));
	at91_add_device_spi(0, &spi_pdata);
}

/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PB16,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

struct gpio_led leds[] = {
	{
		.gpio	= AT91_PIN_PB4,
		.led	= {
			.name = "d8",
		},
	}, {
		.gpio	= AT91_PIN_PB5,
		.active_low	= 1,
		.led	= {
			.name = "d9",
		},
	}, {
		.gpio	= AT91_PIN_PB6,
		.led	= {
			.name = "10",
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

static void __init ek_add_device_buttons(void)
{
	at91_set_gpio_input(AT91_PIN_PB3, 1);
	at91_set_deglitch(AT91_PIN_PB3, 1);
	export_env_ull("dfu_button", AT91_PIN_PB3);
}

static int at91sam9n12ek_mem_init(void)
{
	at91_add_device_sdram(128 * 1024 * 1024);

	return 0;
}
mem_initcall(at91sam9n12ek_mem_init);

static int at91sam9n12ek_devices_init(void)
{
	ek_add_device_spi();
	ek_add_device_nand();
	ek_add_device_mci();
	ek_add_led();
	at91_add_device_udc(&ek_udc_data);
	ek_add_device_i2c();
	ek_add_device_ks8851();
	ek_add_device_buttons();

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(CONFIG_MACH_AT91SAM9N12EK);

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
device_initcall(at91sam9n12ek_devices_init);

static int at91sam9n12ek_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91sam9n12ek_console_init);
