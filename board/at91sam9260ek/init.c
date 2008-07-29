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
#include <asm/arch/memory-map.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <asm/arch/gpio.h>

#define   MASK_ALE        (1 << 21)       /* our ALE is AD21 */
#define   MASK_CLE        (1 << 22)

static void at91sam9260ek_nand_hwcontrol(struct nand_chip *this, int cmd)
{
	ulong IO_ADDR_W = (ulong) this->IO_ADDR_W;

	IO_ADDR_W &= ~(MASK_ALE|MASK_CLE);
	switch (cmd) {
	case NAND_CTL_SETCLE:
		IO_ADDR_W |= MASK_CLE;
		break;
	case NAND_CTL_SETALE:
		IO_ADDR_W |= MASK_ALE;
		break;
	case NAND_CTL_CLRNCE:
		at91_set_gpio_value(AT91_PIN_PC14, 1);
		break;
	case NAND_CTL_SETNCE:
		at91_set_gpio_value(AT91_PIN_PC14, 0);
		break;
	}
	this->IO_ADDR_W = (void *) IO_ADDR_W;
}

static int at91sam9260ek_nand_ready(struct nand_chip *this)
{
	return at91_get_gpio_value(AT91_PIN_PC13);
}

static struct nand_platform_data nand_pdata = {
	.hwcontrol = at91sam9260ek_nand_hwcontrol,
	.eccmode = NAND_ECC_SOFT,
	.dev_ready = at91sam9260ek_nand_ready,
	.chip_delay = 20,
};

static struct device_d nand_dev = {
	.name     = "nand_controller",
	.map_base = 0x40000000,
	.size     = 0x10,
	.platform_data = &nand_pdata,
};

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x20000000,
	.size     = 64 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct device_d macb_dev = {
	.name     = "macb",
	.id       = "eth0",
	.map_base = AT91_BASE_EMAC,
	.size     = 0x1000,
	.type     = DEVICE_TYPE_ETHER,
};

static int at91sam9260ek_devices_init(void)
{
	register_device(&sdram_dev);
	register_device(&nand_dev);
	register_device(&macb_dev);

	armlinux_set_bootparams((void *)0x20000100);
	armlinux_set_architecture(MACH_TYPE_AT91SAM9260EK);

	return 0;
}

device_initcall(at91sam9260ek_devices_init);

static struct device_d at91sam9260ek_serial_device = {
	.name     = "atmel_serial",
	.id       = "cs0",
	.map_base = USART3_BASE,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int at91sam9260ek_console_init(void)
{
	register_device(&at91sam9260ek_serial_device);
	return 0;
}

console_initcall(at91sam9260ek_console_init);
