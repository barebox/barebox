/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
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
#include <init.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>
#include <mach/at91sam9_smc.h>
#include <mach/sam9_smc.h>

/*
 * board revision encoding
 * bit 0-3: lcd type
 *	0 => truly TFT1N4633-E (sam9m10g45-ek)
 *	1 => LG philips LB043WQ1 (sam9m10-ekes and sam9g45-ekes)
 */
#define HAVE_LCD_TRULY_TFT1N4633E	(0 << 0)
#define HAVE_LCD_LG_LB043WQ1		(1 << 0)
static void ek_set_board_revision(void)
{
	u32 rev;

#ifdef CONFIG_LCD_LG_LB043WQ1
		rev = HAVE_LCD_LG_LB043WQ1;
#else
		rev = HAVE_LCD_TRULY_TFT1N4633E;
#endif
		armlinux_set_revision(rev);
}

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
/*	.det_pin	= ... not connected */
	.rdy_pin	= AT91_PIN_PC8,
	.enable_pin	= AT91_PIN_PC14,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
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
	sam9_smc_configure(3, &ek_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct at91_ether_platform_data macb_pdata = {
	.flags = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

static int at91sam9m10g45ek_devices_init(void)
{
	at91_add_device_sdram(128 * 1024 * 1024);
	ek_add_device_nand();
	at91_add_device_eth(&macb_pdata);

	devfs_add_partition("nand0", 0x00000, 0x80000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x40000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_6 + 0x100));
	armlinux_set_architecture(MACH_TYPE_AT91SAM9M10G45EK);
	ek_set_board_revision();

	return 0;
}
device_initcall(at91sam9m10g45ek_devices_init);

static int at91sam9m10g45ek_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91sam9m10g45ek_console_init);
