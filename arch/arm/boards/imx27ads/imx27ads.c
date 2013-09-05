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
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx27-regs.h>
#include <asm/armlinux.h>
#include <io.h>
#include <fec.h>
#include <gpio.h>
#include <mach/weim.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <generated/mach-types.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = 1,
};

static int imx27ads_timing_init(void)
{
	/* configure cpld on cs4 */
	imx27_setup_weimcs(4, 0x0000DCF6, 0x444A4541, 0x44443302);

	/* configure synchronous mode for
	 * 16 bit nor flash on cs0 */
	imx27_setup_weimcs(0, 0x0000CC03, 0xa0330D01, 0x00220800);

	writew(0x00f0, 0xc0000000);
	writew(0x00aa, 0xc0000aaa);
	writew(0x0055, 0xc0000554);
	writew(0x00d0, 0xc0000aaa);
	writew(0x66ca, 0xc0000aaa);
	writew(0x00f0, 0xc0000000);

	imx27_setup_weimcs(0, 0x23524E80, 0x10000D03, 0x00720900);

	/* Select FEC data through data path */
	writew(0x0020, MX27_CS4_BASE_ADDR + 0x10);

	/* Enable CPLD FEC data path */
	writew(0x0010, MX27_CS4_BASE_ADDR + 0x14);

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

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC0000000, 32 * 1024 * 1024, 0);

	imx27_add_fec(&fec_info);
	devfs_add_partition("nor0", 0x00000, 0x20000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x20000, 0x20000, DEVFS_PARTITION_FIXED, "env0");
	protect_file("/dev/env0", 1);

	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_MX27ADS);

	return 0;
}

device_initcall(mx27ads_devices_init);

static int mx27ads_console_init(void)
{
	barebox_set_model("Freescale i.MX27 ADS");
	barebox_set_hostname("mx27ads");

	imx27_add_uart0();
	return 0;
}

console_initcall(mx27ads_console_init);

