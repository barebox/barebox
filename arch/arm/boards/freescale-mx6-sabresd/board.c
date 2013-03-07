/*
 * Copyright (C) 2013 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on arch/arm/boards/freescale-mx6-sabrelite/board.c
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
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx6-regs.h>
#include <fec.h>
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <linux/phy.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <sizes.h>
#include <net.h>
#include <mach/imx6.h>
#include <mach/devices-imx6.h>
#include <mach/iomux-mx6.h>
#include <mach/gpio.h>
#include <spi/spi.h>
#include <mach/spi.h>
#include <mach/usb.h>

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

#define SABRESD_SD2_CD	IMX_GPIO_NR(2, 2)
#define SABRESD_SD2_WP	IMX_GPIO_NR(2, 3)

#define SABRESD_SD3_CD	IMX_GPIO_NR(2, 0)
#define SABRESD_SD3_WP	IMX_GPIO_NR(2, 1)

static iomux_v3_cfg_t sabresd_pads[] = {
	/* UART1 */
	MX6Q_PAD_CSI0_DAT11__UART1_RXD,
	MX6Q_PAD_CSI0_DAT10__UART1_TXD,

	/* Ethernet */
	MX6Q_PAD_ENET_MDC__ENET_MDC,
	MX6Q_PAD_ENET_MDIO__ENET_MDIO,
	MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK,

	MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC,
	MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0,
	MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1,
	MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2,
	MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3,
	MX6Q_PAD_RGMII_TX_CTL__ENET_RGMII_TX_CTL,

	MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC,
	MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0,
	MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1,
	MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2,
	MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3,
	MX6Q_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,

	MX6Q_PAD_ENET_CRS_DV__GPIO_1_25,	/* AR8031 PHY Reset */
	MX6Q_PAD_GPIO_16__ENET_ANATOP_ETHERNET_REF_OUT,

	/* SD2 */
	MX6Q_PAD_SD2_CLK__USDHC2_CLK,
	MX6Q_PAD_SD2_CMD__USDHC2_CMD,
	MX6Q_PAD_SD2_DAT0__USDHC2_DAT0,
	MX6Q_PAD_SD2_DAT1__USDHC2_DAT1,
	MX6Q_PAD_SD2_DAT2__USDHC2_DAT2,
	MX6Q_PAD_SD2_DAT3__USDHC2_DAT3,
	MX6Q_PAD_NANDF_D4__USDHC2_DAT4,
	MX6Q_PAD_NANDF_D5__USDHC2_DAT5,
	MX6Q_PAD_NANDF_D6__USDHC2_DAT6,
	MX6Q_PAD_NANDF_D7__USDHC2_DAT7,
	MX6Q_PAD_NANDF_D2__GPIO_2_2,	/* CD */
	MX6Q_PAD_NANDF_D3__GPIO_2_3,	/* WP */

	/* SD3 */
	MX6Q_PAD_SD3_CMD__USDHC3_CMD,
	MX6Q_PAD_SD3_CLK__USDHC3_CLK,
	MX6Q_PAD_SD3_DAT0__USDHC3_DAT0,
	MX6Q_PAD_SD3_DAT1__USDHC3_DAT1,
	MX6Q_PAD_SD3_DAT2__USDHC3_DAT2,
	MX6Q_PAD_SD3_DAT3__USDHC3_DAT3,
	MX6Q_PAD_SD3_DAT4__USDHC3_DAT4,
	MX6Q_PAD_SD3_DAT5__USDHC3_DAT5,
	MX6Q_PAD_SD3_DAT6__USDHC3_DAT6,
	MX6Q_PAD_SD3_DAT7__USDHC3_DAT7,
	MX6Q_PAD_NANDF_D0__GPIO_2_0,	/* CD */
	MX6Q_PAD_NANDF_D1__GPIO_2_1,	/* WP */

	/* SD4 */
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

	/* I2C0 */
	MX6Q_PAD_CSI0_DAT8__I2C1_SDA,
	MX6Q_PAD_CSI0_DAT9__I2C1_SCL,

	/* I2C1 */
	MX6Q_PAD_KEY_COL3__I2C2_SCL,
	MX6Q_PAD_KEY_ROW3__I2C2_SDA,

	/* I2C2 */
	MX6Q_PAD_GPIO_3__I2C3_SCL,
	MX6Q_PAD_GPIO_6__I2C3_SDA,
};

static int sabresd_mem_init(void)
{
	arm_add_mem_device("ram0", 0x10000000, SZ_1G);

	return 0;
}
mem_initcall(sabresd_mem_init);

static int ar8031_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(dev, 0xd, 0x7);
	phy_write(dev, 0xe, 0x8016);
	phy_write(dev, 0xd, 0x4007);

	val = phy_read(dev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(dev, 0x1d, 0x5);
	val = phy_read(dev, 0x1e);
	val |= 0x0100;
	phy_write(dev, 0x1e, val);

	return 0;
}

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RGMII,
	.phy_addr = 1,
};

static void sabresd_phy_reset(void)
{
	/* Reset AR8031 PHY */
	gpio_direction_output(IMX_GPIO_NR(1, 25) , 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);
}

static inline int imx6_iim_register_fec_ethaddr(void)
{
	u32 value;
	u8 buf[6];

	value = readl(MX6_OCOTP_BASE_ADDR + 0x630);
	buf[0] = (value >> 8);
	buf[1] = value;

	value = readl(MX6_OCOTP_BASE_ADDR + 0x620);
	buf[2] = value >> 24;
	buf[3] = value >> 16;
	buf[4] = value >> 8;
	buf[5] = value;

	eth_register_ethaddr(0, buf);

	return 0;
}

static struct esdhc_platform_data sabresd_sd2_data = {
	.cd_gpio = SABRESD_SD2_CD,
	.cd_type = ESDHC_CD_GPIO,
	.wp_gpio = SABRESD_SD2_WP,
	.wp_type = ESDHC_WP_GPIO,
};

static struct esdhc_platform_data sabresd_sd3_data = {
	.cd_gpio = SABRESD_SD3_CD,
	.cd_type = ESDHC_CD_GPIO,
	.wp_gpio = SABRESD_SD3_WP,
	.wp_type = ESDHC_WP_GPIO,
};

static struct esdhc_platform_data sabresd_sd4_data = {
	.cd_type = ESDHC_CD_PERMANENT,
	.wp_type = ESDHC_WP_CONTROLLER,
};

static int sabresd_devices_init(void)
{
	imx6_add_mmc3(&sabresd_sd4_data);
	imx6_add_mmc1(&sabresd_sd2_data);
	imx6_add_mmc2(&sabresd_sd3_data);

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK, ar8031_phy_fixup);

	sabresd_phy_reset();
	imx6_iim_register_fec_ethaddr();
	imx6_add_fec(&fec_info);

	armlinux_set_bootparams((void *)0x10000100);
	armlinux_set_architecture(3980);

	devfs_add_partition("disk0", 0, SZ_1M, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", SZ_1M + SZ_1M, SZ_512K, DEVFS_PARTITION_FIXED, "env0");
	return 0;
}
device_initcall(sabresd_devices_init);

static int sabresd_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(sabresd_pads, ARRAY_SIZE(sabresd_pads));

	imx6_init_lowlevel();

	imx6_add_uart0();

	return 0;
}
console_initcall(sabresd_console_init);
