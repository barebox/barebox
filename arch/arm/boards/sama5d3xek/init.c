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
#include <mach/at91sam9_smc.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9x5_matrix.h>
#include <input/qt1070.h>
#include <readkey.h>
#include <poller.h>
#include <linux/w1-gpio.h>
#include <w1_mac_address.h>
#include <spi/spi.h>
#include <linux/clk.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>

#include "hw_version.h"

#ifdef CONFIG_W1_MASTER_GPIO
struct w1_gpio_platform_data w1_pdata = {
	.pin = AT91_PIN_PE25,
	.ext_pullup_enable_pin = -EINVAL,
	.is_open_drain = 0,
};
#endif

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

static struct sam9_smc_config cm_nand_smc_config = {
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
		cm_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		cm_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sama5_smc_configure(0, 3, &cm_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}
#else
static void ek_add_device_nand(void) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
static struct macb_platform_data gmac_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RGMII,
	.phy_addr = -1,
};

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = -1,
};

static bool used_23 = false;
static bool used_43 = false;

static int ek_register_mac_address_23(int id)
{
	if (used_23)
		return -EBUSY;

	used_23 = true;

	return w1_local_mac_address_register(id, "tml", "w1-23-0");
}

static int ek_register_mac_address_43(int id)
{
	if (used_43)
		return -EBUSY;

	used_43 = true;

	return w1_local_mac_address_register(id, "tml", "w1-43-0");
}

static int ksz9021rn_phy_fixup(struct phy_device *phy)
{
	int value;

#define GMII_RCCPSR	260
#define GMII_RRDPSR	261
#define GMII_ERCR	11
#define GMII_ERDWR	12

	/* Set delay values */
	value = GMII_RCCPSR | 0x8000;
	phy_write(phy, GMII_ERCR, value);
	value = 0xF2F4;
	phy_write(phy, GMII_ERDWR, value);
	value = GMII_RRDPSR | 0x8000;
	phy_write(phy, GMII_ERCR, value);
	value = 0x2222;
	phy_write(phy, GMII_ERDWR, value);

	return 0;
}

static void ek_add_device_eth(void)
{
	if (w1_local_mac_address_register(0, "tml", "w1-2d-0"))
		if (ek_register_mac_address_23(0))
			ek_register_mac_address_43(0);

	if (ek_register_mac_address_23(1))
		ek_register_mac_address_43(1);

	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);

	at91_add_device_eth(0, &gmac_pdata);
	at91_add_device_eth(1, &macb_pdata);
}
#else
static void ek_add_device_eth(void) {}
#endif

#if defined(CONFIG_DRIVER_VIDEO_ATMEL_HLCD)
/*
 * LCD Controller
 */
static struct fb_videomode at91_tft_vga_modes[] = {
	{
		.name		= "LG",
		.refresh	= 60,
		.xres		= 800,		.yres		= 480,
		.pixclock	= KHZ2PICOS(33260),

		.left_margin	= 88,		.right_margin	= 168,
		.upper_margin	= 8,		.lower_margin	= 37,
		.hsync_len	= 128,		.vsync_len	= 2,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
};

/* Default output mode is TFT 24 bit */
#define BPP_OUT_DEFAULT_LCDCFG5	(LCDC_LCDCFG5_MODE_OUTPUT_24BPP)

/* Driver datas */
static struct atmel_lcdfb_platform_data ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.default_bpp			= 16,
	.default_dmacon			= ATMEL_LCDC_DMAEN,
	.default_lcdcon2		= BPP_OUT_DEFAULT_LCDCFG5,
	.guard_time			= 9,
	.lcd_wiring_mode		= ATMEL_LCDC_WIRING_RGB,
	.mode_list			= at91_tft_vga_modes,
	.num_modes			= ARRAY_SIZE(at91_tft_vga_modes),
};

static void ek_add_device_lcdc(void)
{
	at91_add_device_lcdc(&ek_lcdc_data);
}

#else
static void ek_add_device_lcdc(void) {}
#endif

#if defined(CONFIG_MCI_ATMEL)
/*
 * MCI (SD/MMC)
 */
static struct atmel_mci_platform_data mci0_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PD17,
	.wp_pin		= -EINVAL,
};

static struct atmel_mci_platform_data mci1_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PD18,
	.wp_pin		= -EINVAL,
};

static void ek_add_device_mci(void)
{
	/* MMC0 */
	at91_add_device_mci(0, &mci0_data);
	/* MMC1 */
	at91_add_device_mci(1, &mci1_data);
}
#else
static void ek_add_device_mci(void) {}
#endif

#if defined(CONFIG_I2C_GPIO)
struct qt1070_platform_data qt1070_pdata = {
	.irq_pin	= AT91_PIN_PE31,
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
	at91_add_device_i2c(1, i2c_devices, ARRAY_SIZE(i2c_devices));
	at91_add_device_i2c(0, NULL, 0);
}
#else
static void ek_add_device_i2c(void) {}
#endif

#if defined(CONFIG_DRIVER_SPI_ATMEL)
static const struct spi_board_info ek_spi_devices[] = {
	{
		.name		= "m25p80",
		.chip_select	= 0,
		.max_speed_hz	= 30 * 1000 * 1000,
		.bus_num	= 0,
	}
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PD13 };
static struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void ek_add_device_spi(void)
{
	spi_register_board_info(ek_spi_devices, ARRAY_SIZE(ek_spi_devices));
	at91_add_device_spi(0, &spi_pdata);
}
#else
static void ek_add_device_spi(void) {}
#endif

#ifdef CONFIG_LED_GPIO
struct gpio_led leds[] = {
	{
		.gpio	= AT91_PIN_PE24,
		.active_low	= 1,
		.led	= {
			.name = "d1",
		},
	}, {
#ifndef CONFIG_W1_MASTER_GPIO
		.gpio	= AT91_PIN_PE25,
		.active_low	= 1,
		.led	= {
			.name = "d2",
		},
#endif
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

static int at91sama5d3xek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91sama5d3xek_mem_init);

#ifdef CONFIG_W1_MASTER_GPIO
static void ek_add_device_w1(void)
{
	at91_set_gpio_input(w1_pdata.pin, 0);
	at91_set_multi_drive(w1_pdata.pin, 1);
	add_generic_device_res("w1-gpio", DEVICE_ID_SINGLE, NULL, 0, &w1_pdata);

	at91sama5d3xek_devices_detect_hw();
}
#else
static void ek_add_device_w1(void) {}
#endif

#ifdef CONFIG_POLLER
/*
 * The SiI9022A (HDMI) and QT1070 share the same irq
 * but if the SiI9022A is not reset the irq is pull down
 * So do it. As the SiI9022A need 1s to reset (500ms up then 500ms down then up)
 * do it poller to do not slow down the boot
 */
static int hdmi_reset_pin = AT91_PIN_PC31;
static uint64_t hdmi_reset_start;
struct poller_struct hdmi_poller;

static void hdmi_on_poller(struct poller_struct *poller)
{
	if (!is_timeout_non_interruptible(hdmi_reset_start, 500 * MSECOND))
		return;

	gpio_set_value(hdmi_reset_pin, 1);

	poller_unregister(poller);
	ek_add_device_i2c();
}

static void hdmi_off_poller(struct poller_struct *poller)
{
	if (!is_timeout_non_interruptible(hdmi_reset_start, 500 * MSECOND))
		return;

	gpio_set_value(hdmi_reset_pin, 0);

	hdmi_reset_start = get_time_ns();
	poller->func = hdmi_on_poller;
}

static void ek_add_device_hdmi(void)
{
	at91_set_gpio_output(hdmi_reset_pin, 1);
	hdmi_reset_start = get_time_ns();
	hdmi_poller.func = hdmi_off_poller;

	poller_register(&hdmi_poller);
}
#else
static void ek_add_device_hdmi(void)
{
	ek_add_device_i2c();
}
#endif

static const struct devfs_partition at91sama5d3xek_nand0_partitions[] = {
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

static int at91sama5d3xek_devices_init(void)
{
	ek_add_device_w1();
	ek_add_device_hdmi();
	ek_add_device_nand();
	ek_add_led();
	ek_add_device_eth();
	ek_add_device_spi();
	ek_add_device_mci();
	ek_add_device_lcdc();

	devfs_create_partitions("nand0", at91sama5d3xek_nand0_partitions);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_sama5d3xek);

	return 0;
}
device_initcall(at91sama5d3xek_devices_init);

static int at91sama5d3xek_console_init(void)
{
	barebox_set_model("Atmel sama5d3x-ek");
	barebox_set_hostname("sama5d3x-ek");

	at91_register_uart(0, 0);
	at91_register_uart(2, 0);
	return 0;
}
console_initcall(at91sama5d3xek_console_init);

static int at91sama5d3xek_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(at91sama5d3xek_main_clock);
