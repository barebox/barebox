/*
 * SAMA5D4EK board configureation.
 *
 * Copyright (C) 2014 Atmel Corporation,
 *		      Bo Shen <voice.shen@atmel.com>
 *
 * Licensed under GPLv2 or later.
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <mach/hardware.h>
#include <nand.h>
#include <sizes.h>
#include <linux/mtd/nand.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/at91sam9x5_matrix.h>
#include <input/qt1070.h>
#include <readkey.h>
#include <spi/spi.h>
#include <linux/clk.h>

#if defined(CONFIG_NAND_ATMEL)
static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= -EINVAL,
	.enable_pin	= -EINVAL,
	.ecc_mode	= NAND_ECC_HW,
	.pmecc_sector_size = 512,
	.pmecc_corr_cap = 8,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config cm_nand_smc_config = {
	.ncs_read_setup		= 1,
	.nrd_setup		= 1,
	.ncs_write_setup	= 1,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 3,
	.nrd_pulse		= 2,
	.ncs_write_pulse	= 3,
	.nwe_pulse		= 2,

	.read_cycle		= 5,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 3,

	.tclr			= 2,
	.tadl			= 7,
	.tar			= 2,
	.ocms			= 0,
	.trr			= 3,
	.twb			= 7,
	.rbnsel			= 3,
	.nfsel			= 1,
};

static void ek_add_device_nand(void)
{
	struct clk *clk = clk_get(NULL, "smc_clk");

	clk_enable(clk);

	/* configure chip-select 3 (NAND) */
	sama5_smc_configure(0, 3, &cm_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}
#else
static void ek_add_device_nand(void) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
static struct macb_platform_data macb0_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static void ek_add_device_eth(void)
{
	at91_add_device_eth(0, &macb0_pdata);
}
#else
static void ek_add_device_eth(void) {}
#endif

#if defined(CONFIG_DRIVER_VIDEO_ATMEL_HLCD)
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

/* Output mode is TFT 18 bits */
#define BPP_OUT_DEFAULT_LCDCFG5	(LCDC_LCDCFG5_MODE_OUTPUT_18BPP)

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
static struct atmel_mci_platform_data mci1_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PE6,
	.wp_pin		= -EINVAL,
};

static void ek_add_device_mci(void)
{
	/* MMC1 */
	at91_add_device_mci(1, &mci1_data);

	/* power on MCI1 */
	at91_set_gpio_output(AT91_PIN_PE15, 0);
}
#else
static void ek_add_device_mci(void) {}
#endif

#if defined(CONFIG_I2C_GPIO)
struct qt1070_platform_data qt1070_pdata = {
	.irq_pin	= AT91_PIN_PE25,
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

static unsigned spi0_standard_cs[] = { AT91_PIN_PC3 };
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
		.gpio	= AT91_PIN_PE28,
		.active_low	= 0,
		.led	= {
			.name = "d8",
		},
	}, {
		.gpio	= AT91_PIN_PE9,
		.active_low	= 1,
		.led	= {
			.name = "d9",
		},
	}, {
		.gpio	= AT91_PIN_PE8,
		.active_low	= 0,
		.led	= {
			.name = "d10",
		},
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

static int sama5d4ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(sama5d4ek_mem_init);

static const struct devfs_partition sama5d4ek_nand0_partitions[] = {
	{
		.offset = 0x00000,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "at91bootstrap_raw",
		.bbname = "at91bootstrap",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_512K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "bootloader_raw",
		.bbname = "bootloader",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw",
		.bbname = "env0",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw1",
		.bbname = "env1",
	}, {
		/* sentinel */
	}
};

static int sama5d4ek_devices_init(void)
{
	ek_add_device_i2c();
	ek_add_device_nand();
	ek_add_led();
	ek_add_device_eth();
	ek_add_device_spi();
	ek_add_device_mci();
	ek_add_device_lcdc();

	devfs_create_partitions("nand0", sama5d4ek_nand0_partitions);

	return 0;
}
device_initcall(sama5d4ek_devices_init);

static int sama5d4ek_console_init(void)
{
	barebox_set_model("Atmel sama5d4ek");
	barebox_set_hostname("sama5d4ek");

	at91_register_uart(4, 0);

	return 0;
}
console_initcall(sama5d4ek_console_init);

static int sama5d4ek_main_clock(void)
{
	at91_set_main_clock(12000000);

	return 0;
}
pure_initcall(sama5d4ek_main_clock);
