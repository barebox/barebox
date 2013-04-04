/*
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
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
 * Foundation.
 *
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx6-regs.h>
#include <asm/armlinux.h>
#include <fec.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <spi/spi.h>
#include <sizes.h>
#include <gpio.h>
#include <mci.h>
#include <bootsource.h>
#include <mfd/stmpe-i2c.h>
#include <linux/micrel_phy.h>

#include <asm/io.h>
#include <asm/mmu.h>

#include <mach/devices-imx6.h>
#include <mach/iomux-mx6.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6-regs.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/bbu.h>
#include <mach/spi.h>

static iomux_v3_cfg_t realq7_pads[] = {
	MX6Q_PAD_SD2_CMD__AUDMUX_AUD4_RXC,
	MX6Q_PAD_SD2_DAT0__AUDMUX_AUD4_RXD,
	MX6Q_PAD_SD2_CLK__AUDMUX_AUD4_RXFS,
	MX6Q_PAD_SD2_DAT2__AUDMUX_AUD4_TXD,
	MX6Q_PAD_SD2_DAT1__AUDMUX_AUD4_TXFS,
	MX6Q_PAD_KEY_ROW2__CAN1_RXCAN,
	MX6Q_PAD_GPIO_7__CAN1_TXCAN,
	MX6Q_PAD_CSI0_VSYNC__CHEETAH_TRACE_0,
	MX6Q_PAD_CSI0_DAT4__CHEETAH_TRACE_1,
	MX6Q_PAD_CSI0_DAT13__CHEETAH_TRACE_10,
	MX6Q_PAD_CSI0_DAT14__CHEETAH_TRACE_11,
	MX6Q_PAD_CSI0_DAT15__CHEETAH_TRACE_12,
	MX6Q_PAD_CSI0_DAT16__CHEETAH_TRACE_13,
	MX6Q_PAD_CSI0_DAT17__CHEETAH_TRACE_14,
	MX6Q_PAD_CSI0_DAT18__CHEETAH_TRACE_15,
	MX6Q_PAD_CSI0_DAT5__CHEETAH_TRACE_2,
	MX6Q_PAD_CSI0_DAT6__CHEETAH_TRACE_3,
	MX6Q_PAD_CSI0_DAT7__CHEETAH_TRACE_4,
	MX6Q_PAD_CSI0_DAT8__CHEETAH_TRACE_5,
	MX6Q_PAD_CSI0_DAT9__CHEETAH_TRACE_6,
	MX6Q_PAD_CSI0_DAT10__CHEETAH_TRACE_7,
	MX6Q_PAD_CSI0_DAT11__CHEETAH_TRACE_8,
	MX6Q_PAD_CSI0_DAT12__CHEETAH_TRACE_9,
	MX6Q_PAD_CSI0_DATA_EN__CHEETAH_TRCLK,
	MX6Q_PAD_CSI0_MCLK__CHEETAH_TRCTL,
	MX6Q_PAD_EIM_OE__ECSPI2_MISO,
	MX6Q_PAD_EIM_CS1__ECSPI2_MOSI,
	MX6Q_PAD_EIM_CS0__ECSPI2_SCLK,
	MX6Q_PAD_EIM_D24__ECSPI2_SS2,
	MX6Q_PAD_EIM_D25__ECSPI2_SS3,
	MX6Q_PAD_SD1_DAT0__ECSPI5_MISO,
	MX6Q_PAD_SD1_CMD__ECSPI5_MOSI,
	MX6Q_PAD_SD1_CLK__ECSPI5_SCLK,
	MX6Q_PAD_SD2_DAT3__GPIO_1_12,
	MX6Q_PAD_ENET_MDC__ENET_MDC,
	MX6Q_PAD_ENET_MDIO__ENET_MDIO,
	/* IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE_RGMII = 0x80000, done in flash_header.c */
	MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0,
	MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1,
	MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2,
	MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3,
	MX6Q_PAD_RGMII_TX_CTL__ENET_RGMII_TX_CTL,
	MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC,
	MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK,
	MX6Q_PAD_GPIO_0__GPIO_1_0,
	MX6Q_PAD_GPIO_2__GPIO_1_2,
	MX6Q_PAD_ENET_CRS_DV__GPIO_1_25,
	MX6Q_PAD_ENET_RXD0__GPIO_1_27,
	MX6Q_PAD_ENET_TX_EN__GPIO_1_28,
	MX6Q_PAD_GPIO_3__GPIO_1_3,
	MX6Q_PAD_GPIO_4__GPIO_1_4,
	MX6Q_PAD_GPIO_5__GPIO_1_5,
	MX6Q_PAD_GPIO_8__GPIO_1_8,
	MX6Q_PAD_GPIO_9__GPIO_1_9,
	MX6Q_PAD_NANDF_D0__GPIO_2_0,
	MX6Q_PAD_NANDF_D1__GPIO_2_1,
	MX6Q_PAD_NANDF_D2__GPIO_2_2,
	MX6Q_PAD_EIM_A17__GPIO_2_21,
	MX6Q_PAD_EIM_A16__GPIO_2_22,
	MX6Q_PAD_EIM_LBA__GPIO_2_27,
	MX6Q_PAD_NANDF_D3__GPIO_2_3,
	MX6Q_PAD_NANDF_D4__GPIO_2_4,
	MX6Q_PAD_NANDF_D5__GPIO_2_5,
	MX6Q_PAD_NANDF_D6__GPIO_2_6,
	MX6Q_PAD_NANDF_D7__GPIO_2_7,
	MX6Q_PAD_EIM_DA10__GPIO_3_10,
	MX6Q_PAD_EIM_DA11__GPIO_3_11,
	MX6Q_PAD_EIM_DA12__GPIO_3_12,
	MX6Q_PAD_EIM_DA13__GPIO_3_13,
	MX6Q_PAD_EIM_DA14__GPIO_3_14,
	MX6Q_PAD_EIM_DA15__GPIO_3_15,
	MX6Q_PAD_EIM_D16__GPIO_3_16,
	MX6Q_PAD_EIM_D18__GPIO_3_18,
	MX6Q_PAD_EIM_D19__GPIO_3_19,
	MX6Q_PAD_EIM_D20__GPIO_3_20,
	MX6Q_PAD_EIM_D23__GPIO_3_23,
	MX6Q_PAD_EIM_D29__GPIO_3_29,
	MX6Q_PAD_EIM_D30__GPIO_3_30,
	MX6Q_PAD_EIM_DA8__GPIO_3_8,
	MX6Q_PAD_EIM_DA9__GPIO_3_9,
	MX6Q_PAD_KEY_COL2__GPIO_4_10,
	MX6Q_PAD_KEY_COL4__GPIO_4_14,
	MX6Q_PAD_KEY_ROW4__GPIO_4_15,
	MX6Q_PAD_GPIO_19__GPIO_4_5,
	MX6Q_PAD_KEY_COL0__GPIO_4_6,
	MX6Q_PAD_KEY_ROW0__GPIO_4_7,
	MX6Q_PAD_KEY_COL1__GPIO_4_8,
	MX6Q_PAD_KEY_ROW1__GPIO_4_9,
	MX6Q_PAD_EIM_WAIT__GPIO_5_0,
	MX6Q_PAD_EIM_A25__GPIO_5_2,
	MX6Q_PAD_EIM_A24__GPIO_5_4,
	MX6Q_PAD_EIM_BCLK__GPIO_6_31,
	MX6Q_PAD_SD3_DAT5__GPIO_7_0,
	MX6Q_PAD_SD3_DAT4__GPIO_7_1,
	MX6Q_PAD_GPIO_17__GPIO_7_12,
	MX6Q_PAD_GPIO_18__GPIO_7_13,
	MX6Q_PAD_SD3_RST__GPIO_7_8,
	MX6Q_PAD_EIM_D21__I2C1_SCL,
	MX6Q_PAD_EIM_D28__I2C1_SDA,
	MX6Q_PAD_EIM_EB2__I2C2_SCL,
	MX6Q_PAD_KEY_ROW3__I2C2_SDA,
	MX6Q_PAD_EIM_D17__I2C3_SCL,
	MX6Q_PAD_GPIO_6__I2C3_SDA,
	MX6Q_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6Q_PAD_DI0_PIN15__IPU1_DI0_PIN15,
	MX6Q_PAD_DI0_PIN2__IPU1_DI0_PIN2,
	MX6Q_PAD_DI0_PIN3__IPU1_DI0_PIN3,
	MX6Q_PAD_DI0_PIN4__IPU1_DI0_PIN4,
	MX6Q_PAD_DISP0_DAT0__IPU1_DISP0_DAT_0,
	MX6Q_PAD_DISP0_DAT1__IPU1_DISP0_DAT_1,
	MX6Q_PAD_DISP0_DAT10__IPU1_DISP0_DAT_10,
	MX6Q_PAD_DISP0_DAT11__IPU1_DISP0_DAT_11,
	MX6Q_PAD_DISP0_DAT12__IPU1_DISP0_DAT_12,
	MX6Q_PAD_DISP0_DAT13__IPU1_DISP0_DAT_13,
	MX6Q_PAD_DISP0_DAT14__IPU1_DISP0_DAT_14,
	MX6Q_PAD_DISP0_DAT15__IPU1_DISP0_DAT_15,
	MX6Q_PAD_DISP0_DAT16__IPU1_DISP0_DAT_16,
	MX6Q_PAD_DISP0_DAT17__IPU1_DISP0_DAT_17,
	MX6Q_PAD_DISP0_DAT18__IPU1_DISP0_DAT_18,
	MX6Q_PAD_DISP0_DAT19__IPU1_DISP0_DAT_19,
	MX6Q_PAD_DISP0_DAT2__IPU1_DISP0_DAT_2,
	MX6Q_PAD_DISP0_DAT20__IPU1_DISP0_DAT_20,
	MX6Q_PAD_DISP0_DAT21__IPU1_DISP0_DAT_21,
	MX6Q_PAD_DISP0_DAT22__IPU1_DISP0_DAT_22,
	MX6Q_PAD_DISP0_DAT23__IPU1_DISP0_DAT_23,
	MX6Q_PAD_DISP0_DAT3__IPU1_DISP0_DAT_3,
	MX6Q_PAD_DISP0_DAT4__IPU1_DISP0_DAT_4,
	MX6Q_PAD_DISP0_DAT5__IPU1_DISP0_DAT_5,
	MX6Q_PAD_DISP0_DAT6__IPU1_DISP0_DAT_6,
	MX6Q_PAD_DISP0_DAT7__IPU1_DISP0_DAT_7,
	MX6Q_PAD_DISP0_DAT8__IPU1_DISP0_DAT_8,
	MX6Q_PAD_DISP0_DAT9__IPU1_DISP0_DAT_9,
	MX6Q_PAD_SD1_DAT2__PWM2_PWMO,
	MX6Q_PAD_SD1_DAT1__PWM3_PWMO,
	MX6Q_PAD_GPIO_16__SJC_DE_B,
	MX6Q_PAD_KEY_COL3__SPDIF_IN1,
	MX6Q_PAD_EIM_D22__SPDIF_OUT1,
	MX6Q_PAD_SD3_DAT6__UART1_RXD,
	MX6Q_PAD_SD3_DAT7__UART1_TXD,
	MX6Q_PAD_EIM_D27__UART2_RXD,
	MX6Q_PAD_EIM_D26__UART2_TXD,
	MX6Q_PAD_EIM_D31__GPIO_3_31,
	MX6Q_PAD_SD3_CLK__USDHC3_CLK,
	MX6Q_PAD_SD3_CMD__USDHC3_CMD,
	MX6Q_PAD_SD3_DAT0__USDHC3_DAT0,
	MX6Q_PAD_SD3_DAT1__USDHC3_DAT1,
	MX6Q_PAD_SD3_DAT2__USDHC3_DAT2,
	MX6Q_PAD_SD3_DAT3__USDHC3_DAT3,
	MX6Q_PAD_SD4_CLK__USDHC4_CLK,
	MX6Q_PAD_SD4_CMD__USDHC4_CMD,
	MX6Q_PAD_SD4_DAT0__USDHC4_DAT0,
	MX6Q_PAD_SD4_DAT1__USDHC4_DAT1,
	MX6Q_PAD_SD4_DAT2__USDHC4_DAT2,
	MX6Q_PAD_SD4_DAT3__USDHC4_DAT3,
	MX6Q_PAD_SD4_DAT4__USDHC4_DAT4,
	MX6Q_PAD_SD4_DAT5__USDHC4_DAT5,
	MX6Q_PAD_SD4_DAT6__USDHC4_DAT6,
	MX6Q_PAD_SD4_DAT7__USDHC4_DAT7,
	MX6Q_PAD_NANDF_ALE__USDHC4_RST,
	MX6Q_PAD_NANDF_CS1__GPIO_6_14,
	MX6Q_PAD_NANDF_CS2__GPIO_6_15,
};

static iomux_v3_cfg_t realq7_pads_enet[] = {
	MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC,
	MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0,
	MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1,
	MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2,
	MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3,
	MX6Q_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,
};

#define RQ7_GPIO_ENET_PHYADD2	IMX_GPIO_NR(6, 30)
#define RQ7_GPIO_ENET_MODE0	IMX_GPIO_NR(6, 25)
#define RQ7_GPIO_ENET_MODE1	IMX_GPIO_NR(6, 27)
#define RQ7_GPIO_ENET_MODE2	IMX_GPIO_NR(6, 28)
#define RQ7_GPIO_ENET_MODE3	IMX_GPIO_NR(6, 29)
#define RQ7_GPIO_ENET_EN_CLK125	IMX_GPIO_NR(6, 24)
#define RQ7_GPIO_SD3_CD		IMX_GPIO_NR(6, 14)
#define RQ7_GPIO_SD3_WP		IMX_GPIO_NR(6, 15)

static iomux_v3_cfg_t realq7_pads_gpio[] = {
	MX6Q_PAD_RGMII_RXC__GPIO_6_30,
	MX6Q_PAD_RGMII_RD0__GPIO_6_25,
	MX6Q_PAD_RGMII_RD1__GPIO_6_27,
	MX6Q_PAD_RGMII_RD2__GPIO_6_28,
	MX6Q_PAD_RGMII_RD3__GPIO_6_29,
	MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24,
};

static void mmd_write_reg(struct phy_device *dev, int device, int reg, int val)
{
	phy_write(dev, 0x0d, device);
	phy_write(dev, 0x0e, reg);
	phy_write(dev, 0x0d, (1 << 14) | device);
	phy_write(dev, 0x0e, val);
}

static int ksz9031rn_phy_fixup(struct phy_device *dev)
{
	/*
	 * min rx data delay, max rx/tx clock delay,
	 * min rx/tx control delay
	 */
	mmd_write_reg(dev, 2, 4, 0);
	mmd_write_reg(dev, 2, 5, 0);
	mmd_write_reg(dev, 2, 8, 0x003ff);

	return 0;
}

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RGMII,
	.phy_addr = -1,
};

static void realq7_enet_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(realq7_pads_gpio, ARRAY_SIZE(realq7_pads_gpio));
	gpio_direction_output(RQ7_GPIO_ENET_PHYADD2, 0);
	gpio_direction_output(RQ7_GPIO_ENET_MODE0, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE1, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE2, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE3, 1);
	gpio_direction_output(RQ7_GPIO_ENET_EN_CLK125, 1);

	gpio_direction_output(25, 0);
	mdelay(50);

	gpio_direction_output(25, 1);
	mdelay(50);

	mxc_iomux_v3_setup_multiple_pads(realq7_pads_enet, ARRAY_SIZE(realq7_pads_enet));

	phy_register_fixup_for_uid(PHY_ID_KSZ9031, MICREL_PHY_ID_MASK,
					   ksz9031rn_phy_fixup);

	imx6_add_fec(&fec_info);
}

static int realq7_mem_init(void)
{
	arm_add_mem_device("ram0", 0x10000000, SZ_2G);

	return 0;
}
mem_initcall(realq7_mem_init);

static int realq7_spi_cs[] = { IMX_GPIO_NR(1, 12), };

static struct spi_imx_master realq7_spi_0_data = {
	.chipselect = realq7_spi_cs,
	.num_chipselect = ARRAY_SIZE(realq7_spi_cs),
};

static const struct spi_board_info realq7_spi_board_info[] = {
	{
		.name = "m25p80",
		.max_speed_hz = 40000000,
		.bus_num = 4,
		.chip_select = 0,
	}
};

static struct esdhc_platform_data realq7_emmc_data = {
	.cd_type = ESDHC_CD_PERMANENT,
	.caps = MMC_MODE_8BIT,
	.devname = "emmc",
};

static struct stmpe_platform_data stmpe1_pdata = {
	.gpio_base = 224,
	.blocks = STMPE_BLOCK_GPIO,
};

static struct stmpe_platform_data stmpe2_pdata = {
	.gpio_base = 240,
	.blocks = STMPE_BLOCK_GPIO,
};

static struct i2c_board_info realq7_i2c2_devices[] = {
	{
		I2C_BOARD_INFO("stmpe-i2c", 0x40),
		.platform_data = &stmpe1_pdata,
	}, {
		I2C_BOARD_INFO("stmpe-i2c", 0x44),
		.platform_data = &stmpe2_pdata,
	},
};

static int realq7_devices_init(void)
{
	imx6_add_mmc2(NULL);
	imx6_add_mmc3(&realq7_emmc_data);

	realq7_enet_init();

	i2c_register_board_info(1, realq7_i2c2_devices,
			ARRAY_SIZE(realq7_i2c2_devices));

	imx6_add_i2c0(NULL);
	imx6_add_i2c1(NULL);
	imx6_add_i2c2(NULL);

	spi_register_board_info(realq7_spi_board_info,
			ARRAY_SIZE(realq7_spi_board_info));
	imx6_add_spi4(&realq7_spi_0_data);

	imx6_add_sata();

	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0",
		BBU_HANDLER_FLAG_DEFAULT, NULL, 0, 0x00907000);
	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/disk0",
		0, NULL, 0, 0x00907000);

	return 0;
}
device_initcall(realq7_devices_init);

static int realq7_env_init(void)
{
	char *source_str = NULL;

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		if (!IS_ENABLED(CONFIG_MCI_STARTUP))
			setenv("mci0.probe", "1");
		devfs_add_partition("disk0", 0, SZ_1M, DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("disk0", SZ_1M, SZ_1M, DEVFS_PARTITION_FIXED, "env0");
		source_str = "SD/MMC";
		break;
	case BOOTSOURCE_SPI:
		devfs_add_partition("m25p0", 0, SZ_256K, DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("m25p0", SZ_256K, SZ_256K, DEVFS_PARTITION_FIXED, "env0");
		source_str = "SPI flash";
		break;
	default:
		printf("unknown Bootsource, no persistent environment\n");
		break;
	}

	if (source_str)
		printf("Using environment from %s\n", source_str);

	return 0;
}
late_initcall(realq7_env_init);

static int realq7_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(realq7_pads, ARRAY_SIZE(realq7_pads));

	imx6_init_lowlevel();

	imx6_add_uart1();

	return 0;
}
console_initcall(realq7_console_init);
