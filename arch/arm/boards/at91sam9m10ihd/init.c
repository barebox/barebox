/*
 * Copyright (C) 2009-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2 only
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
#include <io.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <mach/board.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/at91sam9_smc.h>
#include <input/qt1070.h>
#include <readkey.h>
#include <linux/w1-gpio.h>
#include <w1_mac_address.h>
#include <spi/spi.h>

#include "hw_version.h"

struct w1_gpio_platform_data w1_pdata = {
	.pin = AT91_PIN_PB25,
	.ext_pullup_enable_pin = -EINVAL,
	.is_open_drain = 0,
};

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC15,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.bus_width_16	= 0,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config ek_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 3,
};

static void ek_add_device_nand(void)
{
	/* setup bus-width (8 or 16) */
	if (nand_pdata.bus_width_16)
		ek_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		ek_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static void ek_add_device_eth(void)
{
	if (w1_local_mac_address_register(0, "tml", "w1-2d-0"))
		w1_local_mac_address_register(0, "tml", "w1-23-0");

	at91_add_device_eth(0, &macb_pdata);
}

#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data ek_mci0_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PC25,
	.wp_pin         = -EINVAL,
};

static void ek_add_device_mci(void)
{
	at91_add_device_mci(0, &ek_mci0_data);
}
#else
static void ek_add_device_mci(void) {}
#endif

struct qt1070_platform_data qt1070_pdata = {
	.irq_pin	= AT91_PIN_PB19,
	.code		= { BB_KEY_ENTER, BB_KEY_ENTER, BB_KEY_UP, BB_KEY_DOWN, },
	.nb_code	= 4,
};

static struct i2c_board_info i2c_devices[] = {
	{
		.platform_data = &qt1070_pdata,
		I2C_BOARD_INFO("qt1070", 0x1b),
	}, {
		I2C_BOARD_INFO("24c512", 0x51)
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
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	}
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PB3 };
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
 * USB HS Host port (common to OHCI & EHCI)
 */
static struct at91_usbh_data ek_usbh_hs_data = {
	.ports			= 1,
	.vbus_pin		= {AT91_PIN_PC30, -EINVAL},
	.vbus_pin_active_low	= {1, 0},
};

static void ek_add_device_usb(void)
{
	at91_add_device_usbh_ohci(&ek_usbh_hs_data);
	at91_add_device_usbh_ehci(&ek_usbh_hs_data);
}

static int at91sam9m10g45ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91sam9m10g45ek_mem_init);

#if defined(CONFIG_DRIVER_VIDEO_ATMEL)
static struct fb_videomode at91fb_default_monspecs[] = {
	{
		.name		= "MULTEK",
		.refresh	= 60,
		.xres		= 800,		.yres		= 480,
		.pixclock	= KHZ2PICOS(15000),

		.left_margin	= 40,		.right_margin	= 40,
		.upper_margin	= 29,		.lower_margin	= 13,
		.hsync_len	= 48,		.vsync_len	= 3,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
};

#define AT91SAM9G45_DEFAULT_LCDCON2	(ATMEL_LCDC_MEMOR_LITTLE \
					| ATMEL_LCDC_DISTYPE_TFT \
					| ATMEL_LCDC_CLKMOD_ALWAYSACTIVE)

/* Driver datas */
static struct atmel_lcdfb_platform_data ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.default_bpp			= 16,
	.default_dmacon			= ATMEL_LCDC_DMAEN,
	.default_lcdcon2		= AT91SAM9G45_DEFAULT_LCDCON2,
	.guard_time			= 9,
	.lcd_wiring_mode		= ATMEL_LCDC_WIRING_RGB,
	.gpio_power_control		= AT91_PIN_PE6,
	.mode_list			= at91fb_default_monspecs,
	.num_modes			= ARRAY_SIZE(at91fb_default_monspecs),
};

static void ek_add_device_lcd(void)
{
	at91_add_device_lcdc(&ek_lcdc_data);
}
#else
static void ek_add_device_lcd(void) {}
#endif

static void ek_add_device_w1(void)
{
	at91_set_gpio_input(w1_pdata.pin, 0);
	at91_set_multi_drive(w1_pdata.pin, 1);
	add_generic_device_res("w1-gpio", DEVICE_ID_SINGLE, NULL, 0, &w1_pdata);

	at91sam9m10ihd_devices_detect_hw();
}

static int at91sam9m10ihd_devices_init(void)
{
	ek_add_device_w1();
	ek_add_device_nand();
	ek_add_device_eth();
	ek_add_device_mci();
	ek_add_device_spi();
	ek_add_device_i2c();
	ek_add_device_usb();
	ek_add_device_lcd();

	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	/*
	 * The internal Atmel kernel use the SAM9M10G45EK machine id
	 * The mainline use DT
	 */
	armlinux_set_architecture(MACH_TYPE_AT91SAM9M10G45EK);

	return 0;
}
device_initcall(at91sam9m10ihd_devices_init);

static int at91sam9m10ihd_console_init(void)
{
	barebox_set_model("Atmel at91sam9m10ihd");
	barebox_set_hostname("at91sam9m10ihd");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91sam9m10ihd_console_init);

static int at91sam9m10ihd_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(at91sam9m10ihd_main_clock);
