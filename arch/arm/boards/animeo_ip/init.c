/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2 only
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
#include <mach/iomux.h>
#include <mach/io.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>

static bool animeo_ip_is_buco;
static bool animeo_ip_is_io;

static int animeo_ip_get_pio_revision(int gpio, char *name)
{
	int ret;

	ret = gpio_request(gpio, name);
	if (ret) {
		pr_err("animeo_ip: can not request gpio %d as %s (%d)\n",
			gpio, name, ret);
		return ret;
	}

	ret = gpio_direction_input(gpio);

	if (ret) {
		pr_err("animeo_ip: can configure gpio %d (%s) as input (%d)\n",
			gpio, name, ret);
		return ret;
	}

	return gpio_get_value(gpio);
}

static void animeo_ip_detect_version(void)
{
	struct device_d *dev = NULL;
	char *model, *version;
	int val;

	animeo_ip_is_io = false;
	animeo_ip_is_buco = false;
	model = "SubCo";
	version = "SDN";

	dev = add_generic_device_res("animeo_ip", DEVICE_ID_SINGLE, NULL, 0, NULL);

	val = animeo_ip_get_pio_revision(AT91_PIN_PC20, "is_buco");
	if (val < 0) {
		pr_warn("Can not detect model use %s by default\n", model);
	} else if (val) {
		animeo_ip_is_buco = true;
		model = "SubCo";
	}

	val = animeo_ip_get_pio_revision(AT91_PIN_PC21, "is_io");
	if (val < 0) {
		pr_warn("Can not detect version use %s by default\n", model);
	} else if (val) {
		animeo_ip_is_io = true;
		version = "IO";
	}

	dev_add_param_fixed(dev, "model", model);
	dev_add_param_fixed(dev, "version", version);
}

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.bus_width_16	= 0,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config animeo_ip_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 2,
};

static void animeo_ip_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &animeo_ip_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct macb_platform_data macb_pdata = {
	.phy_addr = 0,
};

/*
 * MCI (SD/MMC)
 */
#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata animeo_ip_mci_data = {
	.bus_width	= 4,
	.slot_b		= 1,
	.detect_pin	= -EINVAL,
	.wp_pin		= -EINVAL,
};

static void animeo_ip_add_device_mci(void)
{
	at91_add_device_mci(0, &animeo_ip_mci_data);
}
#else
static void animeo_ip_add_device_mci(void) {}
#endif

struct gpio_bicolor_led leds[] = {
	{
		.gpio_c0 = AT91_PIN_PC17,
		.gpio_c1 = AT91_PIN_PA2,
		.led	= {
			.name = "power_red_green",
		},
	}, {
		.gpio_c0 = AT91_PIN_PC19,
		.gpio_c1 = AT91_PIN_PC18,
		.led	= {
			.name = "tx_greem_rx_yellow",
		},
	},
};

static void __init animeo_ip_add_device_led(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		at91_set_gpio_output(leds[i].gpio_c0, leds[i].active_low);
		at91_set_gpio_output(leds[i].gpio_c1, leds[i].active_low);
		led_gpio_bicolor_register(&leds[i]);
	}
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &leds[0].led);
}

static int animeo_ip_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(animeo_ip_mem_init);

static void animeo_export_gpio_in(int gpio, const char *name)
{
	at91_set_gpio_input(gpio, 0);
	at91_set_deglitch(gpio, 1);
	export_env_ull(name, gpio);
}

static void animeo_ip_add_device_buttons(void)
{
	animeo_export_gpio_in(AT91_PIN_PB1,  "keyswitch_in");
	animeo_export_gpio_in(AT91_PIN_PB2,  "error_in");
	animeo_export_gpio_in(AT91_PIN_PC22, "jumper");
	animeo_export_gpio_in(AT91_PIN_PC23, "btn");
}

static void animeo_export_gpio_out(int gpio, const char *name)
{
	at91_set_gpio_output(gpio, 0);
	export_env_ull(name, gpio);
}

static void animeo_ip_power_control(void)
{
	animeo_export_gpio_out(AT91_PIN_PB0, "power_radio");
	animeo_export_gpio_out(AT91_PIN_PB3, "error_out_relay");
	animeo_export_gpio_out(AT91_PIN_PC4, "power_save");
}

static int animeo_ip_devices_init(void)
{
	animeo_ip_detect_version();
	animeo_ip_power_control();
	animeo_ip_add_device_nand();
	at91_add_device_eth(0, &macb_pdata);
	animeo_ip_add_device_mci();
	animeo_ip_add_device_buttons();
	animeo_ip_add_device_led();

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	/*
	 * in production the machine id used is the cpu module machine id
	 * PICOCOM1
	 */
	armlinux_set_architecture(MACH_TYPE_PICOCOM1);

	devfs_add_partition("nand0", 0x00000, SZ_32K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_32K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_32K, SZ_32K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	return 0;
}

device_initcall(animeo_ip_devices_init);

static int animeo_ip_console_init(void)
{
	/*
	 * disable the dbgu enable by the bootstrap
	 * so linux can detect that we only enable the uart2
	 * and use it for decompress
	 */
#define ATMEL_US_BRGR	0x0020
	at91_sys_write(AT91_DBGU + ATMEL_US_BRGR, 0);
	at91_register_uart(3, 0);
	return 0;
}
console_initcall(animeo_ip_console_init);

static int animeo_ip_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(animeo_ip_main_clock);
