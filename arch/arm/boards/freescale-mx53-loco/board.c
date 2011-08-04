/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2011 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <environment.h>
#include <fcntl.h>
#include <fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <sizes.h>

#include <generated/mach-types.h>

#include <mach/imx-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/gpio.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mmu.h>

static struct fec_platform_data fec_info = {
	.xcv_type = RMII,
};

static struct pad_desc loco_pads[] = {
	/* UART1 */
	MX53_PAD_CSI0_DAT10__UART1_TXD_MUX,
	MX53_PAD_CSI0_DAT11__UART1_RXD_MUX,

	/* FEC */
	MX53_PAD_FEC_MDC__FEC_MDC,
	MX53_PAD_FEC_MDIO__FEC_MDIO,
	MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
	MX53_PAD_FEC_RX_ER__FEC_RX_ER,
	MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
	MX53_PAD_FEC_RXD1__FEC_RDATA_1,
	MX53_PAD_FEC_RXD0__FEC_RDATA_0,
	MX53_PAD_FEC_TX_EN__FEC_TX_EN,
	MX53_PAD_FEC_TXD1__FEC_TDATA_1,
	MX53_PAD_FEC_TXD0__FEC_TDATA_0,
	/* FEC_nRST */
	MX53_PAD_PATA_DA_0__GPIO7_6,

	/* SD1 */
	MX53_PAD_SD1_CMD__ESDHC1_CMD,
	MX53_PAD_SD1_CLK__ESDHC1_CLK,
	MX53_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX53_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX53_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX53_PAD_SD1_DATA3__ESDHC1_DAT3,
	/* SD1_CD */
	MX53_PAD_EIM_DA13__GPIO3_13,
};

static int loco_mem_init(void)
{
	arm_add_mem_device("ram0", 0x70000000, SZ_512M);
	arm_add_mem_device("ram1", 0xb0000000, SZ_512M);

	return 0;
}
mem_initcall(loco_mem_init);

#define LOCO_FEC_PHY_RST		IMX_GPIO_NR(7, 6)

static void loco_fec_reset(void)
{
	gpio_direction_output(LOCO_FEC_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(LOCO_FEC_PHY_RST, 1);
}

static int loco_devices_init(void)
{
	imx51_iim_register_fec_ethaddr();
	imx53_add_fec(&fec_info);
	imx53_add_mmc0(NULL);

	loco_fec_reset();

	armlinux_set_bootparams((void *)0x70000100);
	armlinux_set_architecture(MACH_TYPE_MX53_LOCO);

	loco_fec_reset();

	return 0;
}

device_initcall(loco_devices_init);

static int loco_part_init(void)
{
	devfs_add_partition("disk0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", 0x40000, 0x20000, PARTITION_FIXED, "env0");

	return 0;
}
late_initcall(loco_part_init);

static int loco_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(loco_pads, ARRAY_SIZE(loco_pads));

	imx53_add_uart0();
	return 0;
}

console_initcall(loco_console_init);
