/*
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD
 * Copyright (C) 2014 Gregory Hermant <gregory.hermant@calao-systems.com>
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
#include <gpio.h>
#include <led.h>
#include <mach/io.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <spi/spi.h>
#include <i2c/i2c.h>
#include <libfile.h>

#if defined(CONFIG_NAND_ATMEL)
static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config haba_knx_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
				  AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 3,
};

static void haba_knx_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &haba_knx_nand_smc_config);
	at91_add_device_nand(&nand_pdata);
}
#else
static void haba_knx_add_device_nand(void) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
static struct macb_platform_data macb_pdata = {
	.phy_interface	= PHY_INTERFACE_MODE_RMII,
	.phy_addr	= -1,
};

static void haba_knx_phy_reset(void)
{
	unsigned long rstc;
	struct clk *clk = clk_get(NULL, "macb_clk");

	clk_enable(clk);

	at91_set_gpio_input(AT91_PIN_PA14, 0);
	at91_set_gpio_input(AT91_PIN_PA15, 0);
	at91_set_gpio_input(AT91_PIN_PA17, 0);
	at91_set_gpio_input(AT91_PIN_PA18, 0);

	rstc = at91_sys_read(AT91_RSTC_MR) & AT91_RSTC_ERSTL;

	/* Need to reset PHY -> 500ms reset */
	at91_sys_write(AT91_RSTC_MR, AT91_RSTC_KEY |
				     (AT91_RSTC_ERSTL & (0x0d << 8)) |
				     AT91_RSTC_URSTEN);

	at91_sys_write(AT91_RSTC_CR, AT91_RSTC_KEY | AT91_RSTC_EXTRST);

	/* Wait for end hardware reset */
	while (!(at91_sys_read(AT91_RSTC_SR) & AT91_RSTC_NRSTL))
		;

	/* Restore NRST value */
	at91_sys_write(AT91_RSTC_MR, AT91_RSTC_KEY |
				     (rstc) |
				     AT91_RSTC_URSTEN);
}

#define MACB_SA1B	0x0098
#define MACB_SA1T	0x009c

static int haba_ip_get_macb_ethaddr(u8 *addr)
{
	u32 top, bottom;
	void __iomem *base = IOMEM(AT91SAM9260_BASE_EMAC);

	bottom = readl(base + MACB_SA1B);
	top = readl(base + MACB_SA1T);
	addr[0] = bottom & 0xff;
	addr[1] = (bottom >> 8) & 0xff;
	addr[2] = (bottom >> 16) & 0xff;
	addr[3] = (bottom >> 24) & 0xff;
	addr[4] = top & 0xff;
	addr[5] = (top >> 8) & 0xff;

	/* valid and not private */
	if (is_valid_ether_addr(addr) && !(addr[0] & 0x02))
		return 0;

	return -EINVAL;
}

#define EEPROM_OFFSET 250
#define EEPROM_SIZE 256

static int haba_set_ethaddr(void)
{
	char addr[6];
	char eep_data[256];
	int i, fd, ret;

	fd = open("/dev/eeprom0", O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto err;
	}

	ret = read_full(fd, eep_data, EEPROM_SIZE);
	if (ret < 0)
		goto err_open;

	for (i = 0; i < 6; i++)
		addr[i] = eep_data[EEPROM_OFFSET+i];

	eth_register_ethaddr(0, addr);

	ret = 0;

err_open:
	close(fd);
err:
	if (ret)
		pr_err("can't read eeprom /dev/eeprom0 (%s)\n", strerror(ret));

	return ret;
}

static void haba_knx_add_device_eth(void)
{
	u8 enetaddr[6];

	if (!haba_ip_get_macb_ethaddr(enetaddr))
		eth_register_ethaddr(0, enetaddr);
	else
		haba_set_ethaddr();

	haba_knx_phy_reset();
	at91_add_device_eth(0, &macb_pdata);
}
#else
static void haba_knx_add_device_eth(void) {}
#endif

/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PB2,
	.pullup_pin	= 0,		/* pull-up driven by UDC */
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	}
};

static const struct spi_board_info ek_spi_devices[] = {
	{
		.name		= "m25p16",
		.max_speed_hz	= 30 * 1000 * 1000,
		.bus_num	= 0,
		.chip_select	= 0,
	},
	{
		.name		= "spi_mci",
		.max_speed_hz	= 25 * 1000 * 1000,
		.bus_num	= 0,
		.chip_select	= 1,
	}
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PA3, AT91_PIN_PA27 };
static struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void haba_knx_add_device_spi(void)
{
	if (machine_is_haba_knx_lite()) {
		spi_register_board_info(ek_spi_devices,
					ARRAY_SIZE(ek_spi_devices));
		at91_add_device_spi(0, &spi_pdata);
	}
}

static struct at91_usbh_data haba_knx_usbh_data = {
	.ports		= 2,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

static void haba_knx_add_device_usb(void)
{
	at91_add_device_usbh_ohci(&haba_knx_usbh_data);
}

struct gpio_led led = {
	.gpio = AT91_PIN_PA28,
	.led = {
		.name = "user_led",
	},
};

static void __init ek_add_led(void)
{
	if (!machine_is_haba_knx_lite())
		return;
	at91_set_gpio_output(led.gpio, led.active_low);
	led_gpio_register(&led);
}

static int haba_knx_mem_init(void)
{
	at91_add_device_sdram(0);
	return 0;
}
mem_initcall(haba_knx_mem_init);

static void __init ek_add_device_button(void)
{
	at91_set_GPIO_periph(AT91_PIN_PC3, 1);	/* user btn, pull up enabled */
	at91_set_deglitch(AT91_PIN_PC3, 1);
	export_env_ull("dfu_button", AT91_PIN_PC3);
}

static const struct devfs_partition haba_knx_nand0_partitions[] = {
	{
		.offset = 0x00000,
		.size = SZ_128K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "at91bootstrap_raw",
		.bbname = "at91bootstrap",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_256K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "self_raw",
		.bbname = "self0",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_128K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw",
		.bbname = "env0",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_128K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw1",
		.bbname = "env1",
	}, {
		/* sentinel */
	}
};

static int haba_knx_devices_init(void)
{
	at91_add_device_i2c(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	haba_knx_add_device_nand();
	haba_knx_add_device_eth();
	haba_knx_add_device_spi();
	haba_knx_add_device_usb();
	at91_add_device_udc(&ek_udc_data);
	ek_add_led();
	ek_add_device_button();

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(MACH_TYPE_HABA_KNX_LITE);

	devfs_create_partitions("nand0", haba_knx_nand0_partitions);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_haba_knx);

	return 0;
}
device_initcall(haba_knx_devices_init);

static int haba_knx_console_init(void)
{
	barebox_set_model("CALAO HABA-KNX-LITE");
	barebox_set_hostname("haba-knx-lite");

	at91_register_uart(0, 0);
	at91_set_A_periph(AT91_PIN_PB14, 1);    /* Enable pull-up on DRXD */

	at91_set_gpio_input(AT91_PIN_PB0, 1);	/* Enable pull-up on usd CD */
	return 0;
}
console_initcall(haba_knx_console_init);

static int haba_knx_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(haba_knx_main_clock);
