/*
 * Copyright (C) 2012 Sascha Hauer, Pengutronix
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
#include <bootsource.h>
#include <environment.h>
#include <fcntl.h>
#include <fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <sizes.h>
#include <gpio.h>

#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>
#include <mach/imx5.h>
#include <mach/imx-flash-header.h>
#include <mach/bbu.h>

#include <asm/armlinux.h>
#include <io.h>
#include <asm/mmu.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RMII,
};

static iomux_v3_cfg_t tx53_pads[] = {
	/* UART1 */
	MX53_PAD_PATA_DIOW__UART1_TXD_MUX,
	MX53_PAD_PATA_DMACK__UART1_RXD_MUX,

	MX53_PAD_PATA_DMARQ__UART2_TXD_MUX,
	MX53_PAD_PATA_BUFFER_EN__UART2_RXD_MUX,

	MX53_PAD_PATA_CS_0__UART3_TXD_MUX,
	MX53_PAD_PATA_CS_1__UART3_RXD_MUX,

	/* setup FEC PHY pins for GPIO function */
	MX53_PAD_FEC_REF_CLK__GPIO1_23,
	MX53_PAD_FEC_MDC__GPIO1_31,
	MX53_PAD_FEC_MDIO__GPIO1_22,
	MX53_PAD_FEC_RXD0__GPIO1_27,
	MX53_PAD_FEC_RXD1__GPIO1_26,
	MX53_PAD_FEC_RX_ER__GPIO1_24,
	MX53_PAD_FEC_TX_EN__GPIO1_28,
	MX53_PAD_FEC_TXD0__GPIO1_30,
	MX53_PAD_FEC_TXD1__GPIO1_29,
	MX53_PAD_FEC_CRS_DV__GPIO1_25,

	/* FEC PHY reset */
	MX53_PAD_PATA_DA_0__GPIO7_6,
	/* FEC PHY power */
	MX53_PAD_EIM_D20__GPIO3_20,

	/* SD1 */
	MX53_PAD_SD1_CMD__ESDHC1_CMD,
	MX53_PAD_SD1_CLK__ESDHC1_CLK,
	MX53_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX53_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX53_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX53_PAD_SD1_DATA3__ESDHC1_DAT3,

	/* SD1_CD */
	MX53_PAD_EIM_D24__GPIO3_24,

	MX53_PAD_GPIO_3__I2C3_SCL,
	MX53_PAD_GPIO_6__I2C3_SDA,

	MX53_PAD_CSI0_DAT12__IPU_CSI0_D_12,
	MX53_PAD_CSI0_DAT13__IPU_CSI0_D_13,
	MX53_PAD_CSI0_DAT14__IPU_CSI0_D_14,
	MX53_PAD_CSI0_DAT15__IPU_CSI0_D_15,
	MX53_PAD_CSI0_DAT16__IPU_CSI0_D_16,
	MX53_PAD_CSI0_DAT17__IPU_CSI0_D_17,
	MX53_PAD_CSI0_DAT18__IPU_CSI0_D_18,
	MX53_PAD_CSI0_DAT19__IPU_CSI0_D_19,
	MX53_PAD_CSI0_VSYNC__IPU_CSI0_VSYNC,
	MX53_PAD_CSI0_MCLK__IPU_CSI0_HSYNC,
	MX53_PAD_CSI0_PIXCLK__IPU_CSI0_PIXCLK,
};

#define TX53_SD1_CD			IMX_GPIO_NR(3, 24)

static struct esdhc_platform_data tx53_sd1_data = {
	.cd_gpio = TX53_SD1_CD,
	.cd_type = ESDHC_CD_GPIO,
	.wp_type = ESDHC_WP_NONE,
};

struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

#define FEC_POWER_GPIO		IMX_GPIO_NR(3, 20)
#define FEC_RESET_GPIO		IMX_GPIO_NR(7, 6)

static struct tx53_fec_gpio_setup {
	iomux_v3_cfg_t pad;
	unsigned gpio:9,
		dir:1,
		level:1;
} tx53_fec_gpios[] = {
	{ MX53_PAD_PATA_DA_0__GPIO7_6, FEC_RESET_GPIO,	   1, 0, }, /* PHY reset */
	{ MX53_PAD_EIM_D20__GPIO3_20, FEC_POWER_GPIO,	   1, 1, }, /* PHY power enable */
	{ MX53_PAD_FEC_REF_CLK__GPIO1_23, IMX_GPIO_NR(1, 23), 0, }, /* ENET_CLK */
	{ MX53_PAD_FEC_MDC__GPIO1_31, IMX_GPIO_NR(1, 31), 1, 0, }, /* MDC */
	{ MX53_PAD_FEC_MDIO__GPIO1_22, IMX_GPIO_NR(1, 22), 1, 0, }, /* MDIO */
	{ MX53_PAD_FEC_RXD0__GPIO1_27, IMX_GPIO_NR(1, 27), 1, 1, }, /* Mode0/RXD0 */
	{ MX53_PAD_FEC_RXD1__GPIO1_26, IMX_GPIO_NR(1, 26), 1, 1, }, /* Mode1/RXD1 */
	{ MX53_PAD_FEC_RX_ER__GPIO1_24, IMX_GPIO_NR(1, 24), 0, }, /* RX_ER */
	{ MX53_PAD_FEC_TX_EN__GPIO1_28, IMX_GPIO_NR(1, 28), 1, 0, }, /* TX_EN */
	{ MX53_PAD_FEC_TXD0__GPIO1_30, IMX_GPIO_NR(1, 30), 1, 0, }, /* TXD0 */
	{ MX53_PAD_FEC_TXD1__GPIO1_29, IMX_GPIO_NR(1, 29), 1, 0, }, /* TXD1 */
	{ MX53_PAD_FEC_CRS_DV__GPIO1_25, IMX_GPIO_NR(1, 25), 1, 1, }, /* Mode2/CRS_DV */
};

static iomux_v3_cfg_t tx53_fec_pads[] = {
	MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
	MX53_PAD_FEC_MDC__FEC_MDC,
	MX53_PAD_FEC_MDIO__FEC_MDIO,
	MX53_PAD_FEC_RXD0__FEC_RDATA_0,
	MX53_PAD_FEC_RXD1__FEC_RDATA_1,
	MX53_PAD_FEC_RX_ER__FEC_RX_ER,
	MX53_PAD_FEC_TX_EN__FEC_TX_EN,
	MX53_PAD_FEC_TXD0__FEC_TDATA_0,
	MX53_PAD_FEC_TXD1__FEC_TDATA_1,
	MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
};

static inline void tx53_fec_init(void)
{
	int i;

	/* Configure LAN8700 pads as GPIO and set up
	 * necessary strap options for PHY
	 */
	for (i = 0; i < ARRAY_SIZE(tx53_fec_gpios); i++) {
		struct tx53_fec_gpio_setup *gs = &tx53_fec_gpios[i];

		if (gs->dir)
			gpio_direction_output(gs->gpio, gs->level);
		else
			gpio_direction_input(gs->gpio);

		mxc_iomux_v3_setup_pad(gs->pad);
	}

	/*
	 *Turn on phy power, leave in reset state
	 */
	gpio_set_value(FEC_POWER_GPIO, 1);

	/*
	 * Wait some time to let the phy activate the internal regulator
	 */
	mdelay(10);

	/*
	 * Deassert reset, phy latches the rest of bootstrap pins
	 */
	gpio_set_value(FEC_RESET_GPIO, 1);

	/* LAN7800 has an internal Power On Reset (POR) signal (OR'ed with
	 * the external RESET signal) which is deactivated 21ms after
	 * power on and latches the strap options.
	 * Delay for 22ms to ensure, that the internal POR is inactive
	 * before reconfiguring the strap pins.
	 */
	mdelay(22);

	/*
	 * The phy is ready, now configure imx51 pads for fec operation
	 */
	mxc_iomux_v3_setup_multiple_pads(tx53_fec_pads,
			ARRAY_SIZE(tx53_fec_pads));
}

#define DCD_NAME_1011 static struct imx_dcd_v2_entry dcd_entry_1011

#include "dcd-data-1011.h"

#define DCD_NAME_XX30 static u32 dcd_entry_xx30

#include "dcd-data-xx30.h"

static int tx53_devices_init(void)
{
	imx53_iim_register_fec_ethaddr();
	tx53_fec_init();
	imx53_add_fec(&fec_info);
	imx53_add_mmc0(&tx53_sd1_data);
	imx53_add_nand(&nand_info);

	armlinux_set_bootparams((void *)0x70000100);
	armlinux_set_architecture(MACH_TYPE_TX53);

	/* rev xx30 can boot from nand or USB */
	imx53_bbu_internal_nand_register_handler("nand-xx30",
		BBU_HANDLER_FLAG_DEFAULT, (void *)dcd_entry_xx30,
		sizeof(dcd_entry_xx30), SZ_512K, 0);

	/* rev 1011 can boot from MMC/SD, other bootsource currently unknown */
	imx53_bbu_internal_mmc_register_handler("mmc-1011", "/dev/disk0",
		0, (void *)dcd_entry_1011, sizeof(dcd_entry_1011), 0);

	return 0;
}

device_initcall(tx53_devices_init);

static int tx53_part_init(void)
{
	const char *envdev;

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		devfs_add_partition("disk0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("disk0", SZ_512K, SZ_1M, DEVFS_PARTITION_FIXED, "env0");
		envdev = "MMC";
		break;
	case BOOTSOURCE_NAND:
	default:
		devfs_add_partition("nand0", 0x00000, 0x80000, DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", 0x80000, 0x100000, DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		envdev = "NAND";
		break;
	}

	printf("Using environment in %s\n", envdev);

	return 0;
}
late_initcall(tx53_part_init);

static int tx53_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(tx53_pads, ARRAY_SIZE(tx53_pads));

	if (!IS_ENABLED(CONFIG_TX53_REV_XX30))
		imx53_init_lowlevel(1000);

	barebox_set_model("Ka-Ro TX53");
	barebox_set_hostname("tx53");

	imx53_add_uart0();
	return 0;
}
console_initcall(tx53_console_init);
