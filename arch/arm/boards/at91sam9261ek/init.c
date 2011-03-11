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
#include <dm9000.h>

static struct atmel_nand_data nand_pdata = {
	.ale		= 22,
	.cle		= 21,
/*	.det_pin	= ... not connected */
	.rdy_pin	= AT91_PIN_PC15,
	.enable_pin	= AT91_PIN_PC14,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
};

static struct sam9_smc_config ek_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 2,
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

/*
 * DM9000 ethernet device
 */
#if defined(CONFIG_DRIVER_NET_DM9000)
static struct dm9000_platform_data dm9000_data = {
	.iobase		= AT91_CHIPSELECT_2,
	.iodata		= AT91_CHIPSELECT_2 + 4,
	.buswidth	= DM9000_WIDTH_16,
	.srom		= 0,
};

static struct device_d dm9000_dev = {
	.id		= 0,
	.name		= "dm9000",
	.map_base	= AT91_CHIPSELECT_2,
	.size		= 8,
	.platform_data	= &dm9000_data,
};

/*
 * SMC timings for the DM9000.
 * Note: These timings were calculated for MASTER_CLOCK = 100000000 according to the DM9000 timings.
 */
static struct sam9_smc_config __initdata dm9000_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 8,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 8,
	.nwe_pulse		= 4,

	.read_cycle		= 16,
	.write_cycle		= 16,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_BAT_WRITE | AT91_SMC_DBW_16,
	.tdf_cycles		= 1,
};

static void __init ek_add_device_dm9000(void)
{
	/* Configure chip-select 2 (DM9000) */
	sam9_smc_configure(2, &dm9000_smc_config);

	/* Configure Reset signal as output */
	at91_set_gpio_output(AT91_PIN_PC10, 0);

	/* Configure Interrupt pin as input, no pull-up */
	at91_set_gpio_input(AT91_PIN_PC11, 0);

	register_device(&dm9000_dev);
}
#else
static void __init ek_add_device_dm9000(void) {}
#endif /* CONFIG_DRIVER_NET_DM9000 */

static int at91sam9261ek_devices_init(void)
{

	at91_add_device_sdram(64 * 1024 * 1024);
	ek_add_device_nand();
	ek_add_device_dm9000();

	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	if (machine_is_at91sam9g10ek())
		armlinux_set_architecture(MACH_TYPE_AT91SAM9G10EK);
	else
		armlinux_set_architecture(MACH_TYPE_AT91SAM9261EK);

	return 0;
}

device_initcall(at91sam9261ek_devices_init);

static int at91sam9261ek_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}

console_initcall(at91sam9261ek_console_init);
