/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <environment.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>

/*
 * 32MiB NOR type flash, connected to CS line 0,
 * data width is <friesel>
 */
static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",
	.map_base = IMX_CS0_BASE,
	.size     = 32 * 1024 * 1024,
};

/*
 * ?kiB static RAM type memory, connected to CS4,
 * data width is <friesel>
 */
static struct device_d sram_dev = {
	.name     = "sram",
	.id       = "sram0",
	.map_base = IMX_CS4_BASE,
	.size     = 16 * 1024 * 1024,
};

/*
 * ?MiB NAND type flash
 */
static struct device_d nand_dev = {
	.name     = "cfi_flash_nand",
	.id       = "nand0",
	.map_base = 0x10000000,	/* FIXME */
	.size     = 16 * 1024 * 1024,	/* FIXME */
};

/*
 * SMSC 91xx network controller
 * connected to CS line 1 and interrupt line <blub>,
 * data width is <friesel>
 */
static struct device_d network_dev = {
	.name     = "smsc9xxx",
	.id       = "eth0",
	.map_base = IMX_CS1_BASE,
	.size     = 16 * 1024 * 1024,	/* FIXME */
};

/* 128MiB */
static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = IMX_SDRAM_CS0,
	.size     = 128 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static int imx31_devices_init(void)
{
	__REG(CSCR_U(0)) = 0x0000cf03; /* CS0: Nor Flash */
	__REG(CSCR_L(0)) = 0x10000d03;
	__REG(CSCR_A(0)) = 0x00720900;

	__REG(CSCR_U(1)) = 0x0000df06; /* CS1: Network Controller */
	__REG(CSCR_L(1)) = 0x444a4541;
	__REG(CSCR_A(1)) = 0x44443302;

	__REG(CSCR_U(4)) = 0x0000d843; /* CS4: SRAM */
	__REG(CSCR_L(4)) = 0x22252521;
	__REG(CSCR_A(4)) = 0x22220a00;

	/* setup pins for I2C2 (for EEPROM, RTC) */
	imx_gpio_mode(MUX_CSPI2_MOSI_I2C2_SCL);
	imx_gpio_mode(MUX_CSPI2_MISO_I2C2_SCL);

	register_device(&cfi_dev);
	register_device(&sram_dev);
	register_device(&nand_dev);
	register_device(&network_dev);

	register_device(&sdram_dev);

	return 0;
}

device_initcall(imx31_devices_init);

static struct device_d imx31_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 16 * 1024,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int imx31_console_init(void)
{
	/* init gpios for serial port */
	imx_gpio_mode(MUX_RXD1_UART1_RXD_MUX);
	imx_gpio_mode(MUX_TXD1_UART1_TXD_MUX);
	imx_gpio_mode(MUX_RTS1_UART1_RTS_B);
	imx_gpio_mode(MUX_RTS1_UART1_CTS_B);

	register_device(&imx31_serial_device);
	return 0;
}

console_initcall(imx31_console_init);
