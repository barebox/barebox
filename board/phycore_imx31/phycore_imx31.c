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

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x08000000,		/* FIXME */
	.size     = 16 * 1024 * 1024,	/* FIXME */

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

	/* setup pins for UART1 */
	imx_gpio_mode(MUX_RXD1_UART1_RXD_MUX);
	imx_gpio_mode(MUX_TXD1_UART1_TXD_MUX);
	imx_gpio_mode(MUX_RTS1_UART1_RTS_B);
	imx_gpio_mode(MUX_RTS1_UART1_CTS_B);

	/* setup pins for I2C2 (for EEPROM, RTC) */
	imx_gpio_mode(MUX_CSPI2_MOSI_I2C2_SCL);
	imx_gpio_mode(MUX_CSPI2_MISO_I2C2_SCL);

	return 0;
}

device_initcall(imx31_devices_init);

static struct device_d imx31_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int imx31_console_init(void)
{
	/* init gpios for serial port */
	imx_gpio_mode(PC11_PF_UART1_TXD);
	imx_gpio_mode(PC12_PF_UART1_RXD);

	register_device(&imx31_serial_device);
	return 0;
}

console_initcall(imx31_console_init);
