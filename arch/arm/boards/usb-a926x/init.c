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

static void usb_a9260_set_board_type(void)
{
	if (machine_is_usb_a9g20())
		armlinux_set_architecture(MACH_TYPE_USB_A9G20);
	else if (machine_is_usb_a9263())
		armlinux_set_architecture(MACH_TYPE_USB_A9263);
	else
		armlinux_set_architecture(MACH_TYPE_USB_A9260);
}

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
/*	.det_pin	= ... not connected */
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
};

static struct sam9_smc_config usb_a9260_nand_smc_config = {
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

static struct sam9_smc_config usb_a9g20_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 3,
};

static void usb_a9260_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	if (machine_is_usb_a9g20())
		sam9_smc_configure(3, &usb_a9g20_nand_smc_config);
	else
		sam9_smc_configure(3, &usb_a9260_nand_smc_config);

	if (machine_is_usb_a9263()) {
		nand_pdata.rdy_pin	= AT91_PIN_PA22;
		nand_pdata.enable_pin	= AT91_PIN_PD15;
	}

	at91_add_device_nand(&nand_pdata);
}

static struct at91_ether_platform_data macb_pdata = {
	.flags		= AT91SAM_ETHER_RMII,
	.phy_addr	= 0,
};

static void usb_a9260_phy_reset(void)
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

#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata usb_a9260_mci_data = {
	.bus_width	= 4,
};

static void usb_a9260_add_device_mci(void)
{
	at91_add_device_mci(0, &usb_a9260_mci_data);
}
#else
static void usb_a9260_add_device_mci(void) {}
#endif

static struct at91_usbh_data ek_usbh_data = {
	.ports		= 2,
};

static int usb_a9260_mem_init(void)
{
#ifdef CONFIG_AT91_HAVE_SRAM_128M
	at91_add_device_sdram(128 * 1024 * 1024);
#else
	at91_add_device_sdram(64 * 1024 * 1024);
#endif

	return 0;
}
mem_initcall(usb_a9260_mem_init);

static int usb_a9260_devices_init(void)
{
	usb_a9260_add_device_nand();
	usb_a9260_phy_reset();
	at91_add_device_eth(&macb_pdata);
	usb_a9260_add_device_mci();
	at91_add_device_usbh_ohci(&ek_usbh_data);

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	usb_a9260_set_board_type();

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
device_initcall(usb_a9260_devices_init);

static int usb_a9260_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}
console_initcall(usb_a9260_console_init);
