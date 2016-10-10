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
#include <platform_data/eth-fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <mci.h>
#include <envfs.h>

#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>
#include <mach/imx5.h>

#include <asm/armlinux.h>
#include <io.h>
#include <asm/mmu.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RMII,
};

static iomux_v3_cfg_t smd_pads[] = {
	/* UART1 */
	MX53_PAD_CSI0_DAT10__UART1_TXD_MUX,
	MX53_PAD_CSI0_DAT11__UART1_RXD_MUX,

	/* UART2 */
	MX53_PAD_PATA_BUFFER_EN__UART2_RXD_MUX,
	MX53_PAD_PATA_DMARQ__UART2_TXD_MUX,

	/* UART3 */
	MX53_PAD_PATA_CS_0__UART3_TXD_MUX,
	MX53_PAD_PATA_CS_1__UART3_RXD_MUX,
	MX53_PAD_PATA_DA_1__UART3_CTS,
	MX53_PAD_PATA_DA_2__UART3_RTS,

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
	/* SD1_WP */
	MX53_PAD_KEY_ROW2__GPIO4_11,

	/* SD3 */
	MX53_PAD_PATA_DATA8__ESDHC3_DAT0,
	MX53_PAD_PATA_DATA9__ESDHC3_DAT1,
	MX53_PAD_PATA_DATA10__ESDHC3_DAT2,
	MX53_PAD_PATA_DATA11__ESDHC3_DAT3,
	MX53_PAD_PATA_DATA0__ESDHC3_DAT4,
	MX53_PAD_PATA_DATA1__ESDHC3_DAT5,
	MX53_PAD_PATA_DATA2__ESDHC3_DAT6,
	MX53_PAD_PATA_DATA3__ESDHC3_DAT7,
	MX53_PAD_PATA_IORDY__ESDHC3_CLK,
	MX53_PAD_PATA_RESET_B__ESDHC3_CMD,
};

#define SMD_FEC_PHY_RST		IMX_GPIO_NR(7, 6)

static void smd_fec_reset(void)
{
	gpio_direction_output(SMD_FEC_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(SMD_FEC_PHY_RST, 1);
}

#define LOCO_SD1_CD			IMX_GPIO_NR(3, 13)
#define LOCO_SD1_WP			IMX_GPIO_NR(4, 11)

static struct esdhc_platform_data loco_sd1_data = {
	.cd_gpio = LOCO_SD1_CD,
	.wp_gpio = LOCO_SD1_WP,
	.cd_type = ESDHC_CD_GPIO,
	.wp_type = ESDHC_WP_GPIO,
	.caps    = MMC_CAP_4_BIT_DATA,
};

static struct esdhc_platform_data loco_sd3_data = {
	.wp_type = ESDHC_WP_NONE,
	.cd_type = ESDHC_CD_PERMANENT,
};

static int smd_devices_init(void)
{
	imx53_iim_register_fec_ethaddr();
	imx53_add_fec(&fec_info);
	imx53_add_mmc0(&loco_sd1_data);
	imx53_add_mmc2(&loco_sd3_data);

	smd_fec_reset();

	armlinux_set_architecture(MACH_TYPE_MX53_SMD);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_mx53_smd);

	return 0;
}
device_initcall(smd_devices_init);

static int smd_part_init(void)
{
	devfs_add_partition("disk0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
late_initcall(smd_part_init);

static int smd_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(smd_pads, ARRAY_SIZE(smd_pads));

	barebox_set_model("Freescale i.MX53 SMD");
	barebox_set_hostname("imx53-smd");

	imx53_init_lowlevel(1000);

	imx53_add_uart0();
	imx53_add_uart1();
	imx53_add_uart2();
	return 0;
}
console_initcall(smd_console_init);
