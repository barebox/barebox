/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
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
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <fec.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <asm/arch/board.h>
#include <gpio.h>

#define NAND_READY_GPIO (32 * 2 + 13) /* Port C pin 13 */
#define NAND_ENABLE_GPIO (32 * 2 + 14) /* Port C pin 14 */

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
/*	.det_pin	= ... not connected */
	.ecc_base	= (void __iomem *)AT91C_BASE_HECC,
	.ecc_mode	= NAND_ECC_HW,
	.rdy_pin	= NAND_READY_GPIO,
	.enable_pin	= NAND_ENABLE_GPIO,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
};

static struct at91_ether_platform_data macb_pdata = {
	.flags    = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

static int at91sam9260ek_devices_init(void)
{
	gpio_direction_input(NAND_READY_GPIO);
	gpio_direction_output(NAND_ENABLE_GPIO, 1);

	at91_add_device_nand(&nand_pdata);
	at91_add_device_eth(&macb_pdata);

	at91_add_device_sdram(64 * 1024 * 1024);
	armlinux_set_bootparams((void *)0x20000100);
	armlinux_set_architecture(MACH_TYPE_AT91SAM9260EK);

	devfs_add_partition("nand0", 0x00000, 0x80000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x40000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	return 0;
}

device_initcall(at91sam9260ek_devices_init);

static int at91sam9260ek_console_init(void)
{
	at91_register_uart(0);
	return 0;
}

console_initcall(at91sam9260ek_console_init);
