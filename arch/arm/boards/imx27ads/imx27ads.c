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
#include <mach/imx-regs.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <fec.h>
#include <mach/gpio.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <generated/mach-types.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = 0xC0000000,
	.size     = 32 * 1024 * 1024,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0xa0000000,
	.size     = 128 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
	.phy_addr = 1,
};

static int imx27ads_timing_init(void)
{
	/* configure cpld on cs4 */
	CS4U = 0x0000DCF6;
	CS4L = 0x444A4541;
	CS4A = 0x44443302;

	/* configure synchronous mode for
	 * 16 bit nor flash on cs0 */
	CS0U = 0x0000CC03;
	CS0L = 0xa0330D01;
	CS0A = 0x00220800;

	writew(0x00f0, 0xc0000000);
	writew(0x00aa, 0xc0000aaa);
	writew(0x0055, 0xc0000554);
	writew(0x00d0, 0xc0000aaa);
	writew(0x66ca, 0xc0000aaa);
	writew(0x00f0, 0xc0000000);

	CS0U = 0x23524E80;
	CS0L = 0x10000D03;
	CS0A = 0x00720900;

	/* Select FEC data through data path */
	writew(0x0020, IMX_CS4_BASE + 0x10);

	/* Enable CPLD FEC data path */
	writew(0x0010, IMX_CS4_BASE + 0x14);

	return 0;
}

core_initcall(imx27ads_timing_init);

static int mx27ads_devices_init(void)
{
	int i;
	unsigned int mode[] = {
		PD0_AIN_FEC_TXD0,
		PD1_AIN_FEC_TXD1,
		PD2_AIN_FEC_TXD2,
		PD3_AIN_FEC_TXD3,
		PD4_AOUT_FEC_RX_ER,
		PD5_AOUT_FEC_RXD1,
		PD6_AOUT_FEC_RXD2,
		PD7_AOUT_FEC_RXD3,
		PD8_AF_FEC_MDIO,
		PD9_AIN_FEC_MDC | GPIO_PUEN,
		PD10_AOUT_FEC_CRS,
		PD11_AOUT_FEC_TX_CLK,
		PD12_AOUT_FEC_RXD0,
		PD13_AOUT_FEC_RX_DV,
		PD14_AOUT_FEC_RX_CLK,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
	};

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	register_device(&cfi_dev);
	register_device(&sdram_dev);
	imx27_add_fec(&fec_info);

	devfs_add_partition("nor0", 0x00000, 0x20000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x20000, 0x20000, PARTITION_FIXED, "env0");
	protect_file("/dev/env0", 1);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_MX27ADS);

	return 0;
}

device_initcall(mx27ads_devices_init);

static int mx27ads_console_init(void)
{
	imx27_add_uart0();
	return 0;
}

console_initcall(mx27ads_console_init);

