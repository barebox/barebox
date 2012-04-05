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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <asm/hardware.h>
#include <nand.h>
#include <sizes.h>
#include <linux/mtd/nand.h>
#include <linux/clk.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <mach/sam9_smc.h>
#include <gpio.h>
#include <mach/io.h>
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
	.det_pin	= 0,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
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
	.nwe_pulse		= 2,

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
		sam9_smc_configure(3, &tny_a9g20_nand_smc_config);
	else
		sam9_smc_configure(3, &tny_a9260_nand_smc_config);

	if (machine_is_tny_a9263()) {
		nand_pdata.rdy_pin	= AT91_PIN_PA22;
		nand_pdata.enable_pin	= AT91_PIN_PD15;
	}

	at91_add_device_nand(&nand_pdata);
}

#ifdef CONFIG_DRIVER_NET_MACB
static struct at91_ether_platform_data macb_pdata = {
	.flags		= AT91SAM_ETHER_RMII,
	.phy_addr	= 0,
};

static void tny_a9260_phy_reset(void)
{
	unsigned long rstc;
	struct clk *clk = clk_get(NULL, "macb_clk");

	clk_enable(clk);

	at91_set_gpio_input(AT91_PIN_PA14, 0);
	at91_set_gpio_input(AT91_PIN_PA15, 0);
	at91_set_gpio_input(AT91_PIN_PA17, 0);
	at91_set_gpio_input(AT91_PIN_PA25, 0);
	at91_set_gpio_input(AT91_PIN_PA26, 0);
	at91_set_gpio_input(AT91_PIN_PA28, 0);

	rstc = at91_sys_read(AT91_RSTC_MR) & AT91_RSTC_ERSTL;

	/* Need to reset PHY -> 500ms reset */
	at91_sys_write(AT91_RSTC_MR, AT91_RSTC_KEY |
				     (AT91_RSTC_ERSTL & (0x0d << 8)) |
				     AT91_RSTC_URSTEN);

	at91_sys_write(AT91_RSTC_CR, AT91_RSTC_KEY | AT91_RSTC_EXTRST);

	/* Wait for end hardware reset */
	while (!(at91_sys_read(AT91_RSTC_SR) & AT91_RSTC_NRSTL));

	/* Restore NRST value */
	at91_sys_write(AT91_RSTC_MR, AT91_RSTC_KEY |
				     (rstc) |
				     AT91_RSTC_URSTEN);
}

static void __init ek_add_device_macb(void)
{
	tny_a9260_phy_reset();
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
	.pullup_pin	= 0,		/* pull-up driven by UDC */
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

static int spi0_standard_cs[] = { AT91_PIN_PC11 };
struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void __init ek_add_device_udc(void)
{
	if (machine_is_tny_a9260() || machine_is_tny_a9g20())
		ek_udc_data.vbus_pin = AT91_PIN_PC5;

	at91_add_device_udc(&ek_udc_data);
}

static int tny_a9260_mem_init(void)
{
	at91_add_device_sdram(64 * 1024 * 1024);

	return 0;
}
mem_initcall(tny_a9260_mem_init);

static int tny_a9260_devices_init(void)
{
	tny_a9260_add_device_nand();
	ek_add_device_macb();
	ek_add_device_udc();

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	tny_a9260_set_board_type();

	if (machine_is_tny_a9260() || machine_is_tny_a9g20()) {
		spi_register_board_info(tny_a9g20_spi_devices,
			ARRAY_SIZE(tny_a9g20_spi_devices));
		at91_add_device_spi(0, &spi_pdata);
	}

	devfs_add_partition("nand0", 0x00000, SZ_128K, PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	return 0;
}
device_initcall(tny_a9260_devices_init);

static int tny_a9260_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}
console_initcall(tny_a9260_console_init);
