/*
 * (C) Copyright 2009-2010 Digi International, Inc.
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * (c) 2011 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx51-regs.h>
#include <fec.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <sizes.h>
#include <nand.h>
#include <notifier.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <asm/io.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/iim.h>
#include <mach/clock-imx51_53.h>
#include <mach/imx5.h>
#include <mach/revision.h>
#include <mach/esdctl.h>

#include "ccxmx51.h"

static struct ccxmx51_ident ccxmx51_ids[] = {
/* 0x00 */	{ "Unknown",						0,       0, 0, 0, 0 },
/* 0x01 */	{ "Not supported",					0,       0, 0, 0, 0 },
/* 0x02 */	{ "i.MX515@800MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M, 0, 1, 1, 1 },
/* 0x03 */	{ "i.MX515@800MHz, PHY, Ext. Eth, Accel",		SZ_512M, 0, 1, 1, 0 },
/* 0x04 */	{ "i.MX515@600MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M, 1, 1, 1, 1 },
/* 0x05 */	{ "i.MX515@600MHz, PHY, Ext. Eth, Accel",		SZ_512M, 1, 1, 1, 0 },
/* 0x06 */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_512M, 0, 1, 0, 1 },
/* 0x07 */	{ "i.MX515@800MHz, PHY, Accel",				SZ_512M, 0, 1, 0, 0 },
/* 0x08 */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_256M, 0, 1, 0, 1 },
/* 0x09 */	{ "i.MX515@800MHz, PHY, Accel",				SZ_256M, 0, 1, 0, 0 },
/* 0x0a */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_256M, 1, 1, 0, 1 },
/* 0x0b */	{ "i.MX515@600MHz, PHY, Accel",				SZ_256M, 1, 1, 0, 0 },
/* 0x0c */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_128M, 0, 1, 0, 1 },
/* 0x0d */	{ "i.MX512@800MHz",					SZ_128M, 0, 0, 0, 0 },
/* 0x0e */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_512M, 0, 1, 0, 1 },
/* 0x0f */	{ "i.MX515@600MHz, PHY, Accel",				SZ_128M, 1, 1, 0, 0 },
/* 0x10 */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_128M, 1, 1, 0, 1 },
/* 0x11 */	{ "i.MX515@800MHz, PHY, Accel",				SZ_128M, 0, 1, 0, 0 },
/* 0x12 */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_512M, 1, 1, 0, 1 },
/* 0x13 */	{ "i.MX515@800MHz, PHY, Accel",				SZ_512M, 0, 1, 0, 0 },
};

struct ccxmx51_ident *ccxmx51_id;

struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct fec_platform_data fec_info = {
	.xcv_type	= PHY_INTERFACE_MODE_MII,
	.phy_addr	= 7,
};

static iomux_v3_cfg_t ccxmx51_pads[] = {
	/* UART1 */
	MX51_PAD_UART1_RXD__UART1_RXD,
	MX51_PAD_UART1_TXD__UART1_TXD,
	/* UART2 */
	MX51_PAD_UART2_RXD__UART2_RXD,
	MX51_PAD_UART2_TXD__UART2_TXD,
	/* UART3 */
	MX51_PAD_UART3_RXD__UART3_RXD,
	MX51_PAD_UART3_TXD__UART3_TXD,
	/* I2C2 */
	MX51_PAD_GPIO1_2__I2C2_SCL,
	MX51_PAD_GPIO1_3__I2C2_SDA,
	/* eCSPI1 */
	MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
	MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
	MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,
	MX51_PAD_CSPI1_RDY__ECSPI1_RDY,
	MX51_PAD_CSPI1_SS0__ECSPI1_SS0,
	MX51_PAD_CSPI1_SS1__ECSPI1_SS1,
	/* FEC */
	MX51_PAD_DISP2_DAT14__FEC_RDATA0,
	MX51_PAD_DI2_DISP_CLK__FEC_RDATA1,
	MX51_PAD_DI_GP4__FEC_RDATA2,
	MX51_PAD_DISP2_DAT0__FEC_RDATA3,
	MX51_PAD_DISP2_DAT15__FEC_TDATA0,
	MX51_PAD_DISP2_DAT6__FEC_TDATA1,
	MX51_PAD_DISP2_DAT7__FEC_TDATA2,
	MX51_PAD_DISP2_DAT8__FEC_TDATA3,
	MX51_PAD_DISP2_DAT9__FEC_TX_EN,
	MX51_PAD_DISP2_DAT10__FEC_COL,
	MX51_PAD_DISP2_DAT11__FEC_RX_CLK,
	MX51_PAD_DISP2_DAT12__FEC_RX_DV,
	MX51_PAD_DISP2_DAT13__FEC_TX_CLK,
	MX51_PAD_DI2_PIN2__FEC_MDC,
	MX51_PAD_DI2_PIN4__FEC_CRS,
	MX51_PAD_DI2_PIN3__FEC_MDIO,
	MX51_PAD_DI_GP3__FEC_TX_ER,
	MX51_PAD_DISP2_DAT1__FEC_RX_ER,
	/* WEIM */
	MX51_PAD_EIM_DA0__EIM_DA0,
	MX51_PAD_EIM_DA1__EIM_DA1,
	MX51_PAD_EIM_DA2__EIM_DA2,
	MX51_PAD_EIM_DA3__EIM_DA3,
	MX51_PAD_EIM_DA4__EIM_DA4,
	MX51_PAD_EIM_DA5__EIM_DA5,
	MX51_PAD_EIM_DA6__EIM_DA6,
	MX51_PAD_EIM_DA7__EIM_DA7,
	MX51_PAD_EIM_D16__EIM_D16,
	MX51_PAD_EIM_D17__EIM_D17,
	MX51_PAD_EIM_D18__EIM_D18,
	MX51_PAD_EIM_D19__EIM_D19,
	MX51_PAD_EIM_D20__EIM_D20,
	MX51_PAD_EIM_D21__EIM_D21,
	MX51_PAD_EIM_D22__EIM_D22,
	MX51_PAD_EIM_D23__EIM_D23,
	MX51_PAD_EIM_D24__EIM_D24,
	MX51_PAD_EIM_D25__EIM_D25,
	MX51_PAD_EIM_D26__EIM_D26,
	MX51_PAD_EIM_D27__EIM_D27,
	MX51_PAD_EIM_D28__EIM_D28,
	MX51_PAD_EIM_D29__EIM_D29,
	MX51_PAD_EIM_D30__EIM_D30,
	MX51_PAD_EIM_D31__EIM_D31,
	MX51_PAD_EIM_OE__EIM_OE,
	MX51_PAD_EIM_CS5__EIM_CS5,
	/* NAND */
	MX51_PAD_NANDF_D0__NANDF_D0,
	MX51_PAD_NANDF_D1__NANDF_D1,
	MX51_PAD_NANDF_D2__NANDF_D2,
	MX51_PAD_NANDF_D3__NANDF_D3,
	MX51_PAD_NANDF_D4__NANDF_D4,
	MX51_PAD_NANDF_D5__NANDF_D5,
	MX51_PAD_NANDF_D6__NANDF_D6,
	MX51_PAD_NANDF_D7__NANDF_D7,
	MX51_PAD_NANDF_ALE__NANDF_ALE,
	MX51_PAD_NANDF_CLE__NANDF_CLE,
	MX51_PAD_NANDF_RE_B__NANDF_RE_B,
	MX51_PAD_NANDF_WE_B__NANDF_WE_B,
	MX51_PAD_NANDF_WP_B__NANDF_WP_B,
	MX51_PAD_NANDF_CS0__NANDF_CS0,
	MX51_PAD_NANDF_RB0__NANDF_RB0,
	/* LAN9221 IRQ (GPIO1.9) */
	MX51_PAD_GPIO1_9__GPIO1_9,
	/* MC13892 IRQ (GPIO1.5) */
	MX51_PAD_GPIO1_5__GPIO1_5,
	/* MMA7455LR IRQ1 (GPIO1.7) */
	MX51_PAD_GPIO1_7__GPIO1_7,
	/* MMA7455LR IRQ2 (GPIO1.6) */
	MX51_PAD_GPIO1_6__GPIO1_6,
	/* User GPIOs */
	MX51_PAD_GPIO1_0__GPIO1_0,
	MX51_PAD_GPIO1_1__GPIO1_1,
	MX51_PAD_GPIO1_8__GPIO1_8,
	MX51_PAD_DI1_PIN11__GPIO3_0,
	MX51_PAD_DI1_PIN12__GPIO3_1,
	MX51_PAD_DI1_PIN13__GPIO3_2,
	MX51_PAD_DI1_D0_CS__GPIO3_3,
	MX51_PAD_DI1_D1_CS__GPIO3_4,
	MX51_PAD_DISPB2_SER_DIN__GPIO3_5,
	MX51_PAD_DISPB2_SER_DIO__GPIO3_6,
	MX51_PAD_DISPB2_SER_CLK__GPIO3_7,
	MX51_PAD_DISPB2_SER_RS__GPIO3_8,
	MX51_PAD_NANDF_RB1__GPIO3_9,
	MX51_PAD_NANDF_RB2__GPIO3_10,
	MX51_PAD_NANDF_RB3__GPIO3_11,
	MX51_PAD_CSI1_D8__GPIO3_12,
	MX51_PAD_CSI1_D9__GPIO3_13,
	MX51_PAD_NANDF_CS1__GPIO3_17,
	MX51_PAD_NANDF_CS2__GPIO3_18,
	MX51_PAD_NANDF_CS3__GPIO3_19,
	MX51_PAD_NANDF_CS4__GPIO3_20,
	MX51_PAD_NANDF_CS5__GPIO3_21,
	MX51_PAD_NANDF_CS6__GPIO3_22,
};

#define CCXMX51_ECSPI1_CS0	IMX_GPIO_NR(4, 24)
#define CCXMX51_ECSPI1_CS1	IMX_GPIO_NR(4, 25)

static int ecspi_0_cs[] = { CCXMX51_ECSPI1_CS0, CCXMX51_ECSPI1_CS1, };

static struct spi_imx_master ecspi_0_data = {
	.chipselect	= ecspi_0_cs,
	.num_chipselect	= ARRAY_SIZE(ecspi_0_cs),
};

static const struct spi_board_info ccxmx51_spi_board_info[] = {
	{
		.name		= "mc13892",
		.bus_num	= 0,
		.chip_select	= 0,
	},
};

static struct imxusb_platformdata ccxmx51_otg_pdata = {
	.flags	= MXC_EHCI_MODE_UTMI_16_BIT | MXC_EHCI_INTERNAL_PHY |
		  MXC_EHCI_POWER_PINS_ENABLED,
	.mode	= IMX_USB_MODE_HOST,
};

static int ccxmx51_power_init(void)
{
	struct mc13xxx *mc13xxx_dev;
	u32 val;

	mc13xxx_dev = mc13xxx_get();
	if (!mc13xxx_dev)
		return -ENODEV;

	mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_POWER_MISC, &val);
	/* Reset devices by clearing GP01-GPO4 */
	val &= ~((1 << 21) | (3 << 12) | (3 << 10) | (3 << 8) | (3 << 6));
	/* Switching off the PWGT1SPIEN */
	val |= (1 << 15);
	/* Switching on the PWGT2SPIEN */
	val &= ~(1 << 16);
	/* Enable short circuit protection */
	val |= (1 << 0);
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_POWER_MISC, val);

	/* Allow charger to charge (4.2V and 560mA) */
	val = 0x238033;
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_CHARGE, val);

	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		/* Set core voltage (SW1) to 1.1V */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_0, &val);
		val &= ~0x00001f;
		val |=  0x000014;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_0, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_1, &val);
		val &= ~0x00001f;
		val |=  0x00001a;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_2, &val);
		val &= ~0x00001f;
		val |=  0x00001a;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_2, val);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_1, &val);
		val &= ~0x00001f;
		val |=  0x000019;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_2, &val);
		val &= ~0x00001f;
		val |=  0x000018;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_2, val);
	}

	if (mc13xxx_revision(mc13xxx_dev) <= MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_4, &val);
		val &= ~0x003c0f;
		val |=  0x001405;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_5, &val);
		val &= ~0x000f0f;
		val |=  0x000505;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_5, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_4, &val);
		val &= ~0x003c0f;
		val |=  0x002008;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SW_5, &val);
		val &= ~0x000f0f;
		val |=  0x000808;
		mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SW_5, val);
	}

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SETTING_1, &val);
	val &= ~0x0001fc;
	val |=  0x0001f4;
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x000208;
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_MODE_1, val);
	udelay(200);

	/* Set VGEN3 to 1.8V */
	mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_SETTING_0, &val);
	val &= ~(1 << 14);
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_SETTING_0, val);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x049249;
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_MODE_1, val);

	/* Enable USB1 charger */
	val = 0x000409;
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_USB1, val);

	/* Set VCOIN to 3.0V and Enable It */
	mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_POWER_CTL0, &val);
	val &= ~(7 << 20);
	val |= (4 << 20) | (1 << 23);
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_POWER_CTL0, val);
	/* Keeps VSRTC and CLK32KMCU */
	val |= (1 << 4);
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_POWER_CTL0, val);

	/* De-assert reset of external devices on GP01, GPO2, GPO3 and GPO4 */
	mc13xxx_reg_read(mc13xxx_dev, MC13892_REG_POWER_MISC, &val);
	/* GPO1 - External */
	/* GP02 - LAN9221 Power */
	/* GP03 - FEC Reset */
	/* GP04 - Wireless Power */
	if (IS_ENABLED(CONFIG_DRIVER_NET_SMC911X) && ccxmx51_id->eth1) {
		val |= (1 << 8);
		mdelay(50);
	}
	if (IS_ENABLED(CONFIG_DRIVER_NET_FEC_IMX) && ccxmx51_id->eth0)
		val |= (1 << 10);
	if (ccxmx51_id->wless)
		val |= (1 << 12);
	mc13xxx_reg_write(mc13xxx_dev, MC13892_REG_POWER_MISC, val);

	udelay(100);

	return 0;
}

/*
 * On this board the SDRAM is always configured for 512Mib. The real
 * size is determined by the board id read from the IIM module.
 */
static int ccxmx51_sdram_fixup(void)
{
	imx_esdctl_disable();

	return 0;
}
postcore_initcall(ccxmx51_sdram_fixup);

static int ccxmx51_memory_init(void)
{
	arm_add_mem_device("ram0", MX51_CSD0_BASE_ADDR, SZ_128M);

	return 0;
}
mem_initcall(ccxmx51_memory_init);

static int ccxmx51_devices_init(void)
{
	u8 hwid[6];
	int pwr;
	char manloc;

	if ((imx_iim_read(1, 9, hwid, sizeof(hwid)) != sizeof(hwid)) || (hwid[0] < 0x02) || (hwid[0] >= ARRAY_SIZE(ccxmx51_ids)))
		memset(hwid, 0x00, sizeof(hwid));

	ccxmx51_id = &ccxmx51_ids[hwid[0]];
	printf("Module Variant: %s (0x%02x)\n", ccxmx51_id->id_string, hwid[0]);

	if (hwid[0]) {
		printf("Module HW Rev : %02x\n", hwid[1] + 1);
		switch (hwid[2] & 0xc0) {
		case 0x00:
			manloc = 'B';
			break;
		case 0x40:
			manloc = 'W';
			break;
		case 0x80:
			manloc = 'S';
			break;
		default:
			manloc = 'N';
			break;
		}
		printf("Module Serial : %c%d\n", manloc, ((hwid[2] & 0x3f) << 24) | (hwid[3] << 16) | (hwid[4] << 8) | hwid[5]);
		if ((ccxmx51_id->mem_sz - SZ_128M) > 0)
			arm_add_mem_device("ram1", MX51_CSD0_BASE_ADDR + SZ_128M, ccxmx51_id->mem_sz - SZ_128M);
	} else
		return -ENOSYS;

	imx51_add_uart1();
	imx51_add_uart2();

	spi_register_board_info(ccxmx51_spi_board_info, ARRAY_SIZE(ccxmx51_spi_board_info));
	imx51_add_spi0(&ecspi_0_data);

	pwr = ccxmx51_power_init();
	console_flush();
	imx51_init_lowlevel((ccxmx51_id->industrial || pwr) ? 600 : 800);
	clock_notifier_call_chain();
	if (pwr)
		printf("Could not setup PMIC. Clocks not adjusted.\n");

	imx51_add_i2c1(NULL);

	imx51_add_nand(&nand_info);
	devfs_add_partition("nand0", 0x00000, 0x80000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x80000, 0x40000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	if (IS_ENABLED(CONFIG_DRIVER_NET_FEC_IMX) && ccxmx51_id->eth0 && !pwr) {
		eth_register_ethaddr(0, hwid);
		imx51_add_fec(&fec_info);
	}

	if (IS_ENABLED(CONFIG_DRIVER_NET_SMC911X) && ccxmx51_id->eth1 && !pwr) {
		/* Configure the WEIM CS5 timming, bus width, etc */
		/* 16 bit on DATA[31..16], not multiplexed, async */
		writel(0x00420081, MX51_WEIM_BASE_ADDR + WEIM_CSxGCR1(5));
		/* ADH has not effect on non muxed bus */
		writel(0, MX51_WEIM_BASE_ADDR + WEIM_CSxGCR2(5));
		/* RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
		writel(0x32260000, MX51_WEIM_BASE_ADDR + WEIM_CSxRCR1(5));
		/* APR=0 */
		writel(0, MX51_WEIM_BASE_ADDR + WEIM_CSxRCR2(5));
		/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0, WEN=0, WCSA=0 */
		writel(0x72080f00, MX51_WEIM_BASE_ADDR + WEIM_CSxWCR1(5));

		/* LAN9221 network controller */
		add_generic_device("smc911x", 1, NULL, MX51_CS5_BASE_ADDR, SZ_4K, IORESOURCE_MEM, NULL);
	}

	imx51_add_usbotg(&ccxmx51_otg_pdata);

	armlinux_set_bootparams((void *)(MX51_CSD0_BASE_ADDR + 0x100));

	armlinux_set_architecture(ccxmx51_id->wless ? MACH_TYPE_CCWMX51 : MACH_TYPE_CCMX51);

	return 0;
}
device_initcall(ccxmx51_devices_init);

static int ccxmx51_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(ccxmx51_pads, ARRAY_SIZE(ccxmx51_pads));

	barebox_set_model("Digi ConnectCore i.MX51");
	barebox_set_hostname("ccmx51");

	imx51_add_uart0();

	return 0;
}
console_initcall(ccxmx51_console_init);
