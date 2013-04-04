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
#include <init.h>
#include <environment.h>
#include <mach/imx51-regs.h>
#include <fec.h>
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <mach/bbu.h>
#include <nand.h>
#include <notifier.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <io.h>
#include <asm/mmu.h>
#include <mach/imx5.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/revision.h>
#include <mach/iim.h>
#include <mach/imx-flash-header.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
};

static iomux_v3_cfg_t f3s_pads[] = {
	/* UART1 */
	MX51_PAD_UART1_RXD__UART1_RXD,
	MX51_PAD_UART1_TXD__UART1_TXD,
	MX51_PAD_UART1_RTS__UART1_RTS,
	MX51_PAD_UART1_CTS__UART1_CTS,
	/* FEC */
	MX51_PAD_EIM_EB2__FEC_MDIO,
	MX51_PAD_EIM_EB3__FEC_RDATA1,
	MX51_PAD_EIM_CS2__FEC_RDATA2,
	MX51_PAD_EIM_CS3__FEC_RDATA3,
	MX51_PAD_EIM_CS4__FEC_RX_ER,
	MX51_PAD_EIM_CS5__FEC_CRS,
	MX51_PAD_NANDF_RB2__FEC_COL,
	MX51_PAD_NANDF_RB3__FEC_RX_CLK,
	MX51_PAD_NANDF_CS2__FEC_TX_ER,
	MX51_PAD_NANDF_CS3__FEC_MDC,
	MX51_PAD_NANDF_CS4__FEC_TDATA1,
	MX51_PAD_NANDF_CS5__FEC_TDATA2,
	MX51_PAD_NANDF_CS6__FEC_TDATA3,
	MX51_PAD_NANDF_CS7__FEC_TX_EN,
	MX51_PAD_NANDF_RDY_INT__FEC_TX_CLK,
	MX51_PAD_NANDF_D11__FEC_RX_DV,
	MX51_PAD_NANDF_D9__FEC_RDATA0,
	MX51_PAD_NANDF_D8__FEC_TDATA0,
	MX51_PAD_CSPI1_SS0__ECSPI1_SS0,
	MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
	MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
	MX51_PAD_CSPI1_RDY__ECSPI1_RDY,
	MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,
	MX51_PAD_EIM_A20__GPIO2_14, /* LAN8700 reset pin */
	IOMUX_PAD(0x60C, 0x21C, 3, 0x0, 0, 0x85), /* FIXME: needed? */
	/* SD 1 */
	MX51_PAD_SD1_CMD__SD1_CMD,
	MX51_PAD_SD1_CLK__SD1_CLK,
	MX51_PAD_SD1_DATA0__SD1_DATA0,
	MX51_PAD_SD1_DATA1__SD1_DATA1,
	MX51_PAD_SD1_DATA2__SD1_DATA2,
	MX51_PAD_SD1_DATA3__SD1_DATA3,
	/* SD 2 */
	MX51_PAD_SD2_CMD__SD2_CMD,
	MX51_PAD_SD2_CLK__SD2_CLK,
	MX51_PAD_SD2_DATA0__SD2_DATA0,
	MX51_PAD_SD2_DATA1__SD2_DATA1,
	MX51_PAD_SD2_DATA2__SD2_DATA2,
	MX51_PAD_SD2_DATA3__SD2_DATA3,
	/* CD/WP gpio */
	MX51_PAD_GPIO1_6__GPIO1_6,
	MX51_PAD_GPIO1_5__GPIO1_5,
};

#define BABBAGE_ECSPI1_CS0	(3 * 32 + 24)
static int spi_0_cs[] = {BABBAGE_ECSPI1_CS0};

static struct spi_imx_master spi_0_data = {
	.chipselect = spi_0_cs,
	.num_chipselect = ARRAY_SIZE(spi_0_cs),
};

static const struct spi_board_info mx51_babbage_spi_board_info[] = {
	{
		.name = "mc13xxx-spi",
		.bus_num = 0,
		.chip_select = 0,
	},
};

#define MX51_CCM_CACRR 0x10

static void babbage_power_init(void)
{
	struct mc13xxx *mc13xxx;
	u32 val;

	mc13xxx = mc13xxx_get();
	if (!mc13xxx) {
		printf("could not get PMIC\n");
		return;
	}

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_MISC, &val);
	val &= ~0x10000;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	/* Write needed to update Charger 0 */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_CHARGE, 0x0023807F);

	/* power up the system first */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, 0x00200000);

	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		/* Set core voltage to 1.1V */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_0, &val);
		val &= ~0x1f;
		val |= 0x14;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_0, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x19;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x18;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	}

	if (mc13xxx_revision(mc13xxx) < MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x1405;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x505;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x2008;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x808;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_0, &val);
	val &= ~0x34030;
	val |= 0x10020;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_0, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_1, &val);
	val &= ~0x1FC;
	val |= 0x1F4;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);
	udelay(200);
#define GPIO_LAN8700_RESET	(1 * 32 + 14)

	/* Reset the ethernet controller over GPIO */
	gpio_direction_output(GPIO_LAN8700_RESET, 0);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	gpio_set_value(GPIO_LAN8700_RESET, 1);

	mdelay(50);
}

#define DCD_NAME static struct imx_dcd_entry dcd_entry

#include "dcd-data.h"

static int f3s_devices_init(void)
{
	spi_register_board_info(mx51_babbage_spi_board_info,
			ARRAY_SIZE(mx51_babbage_spi_board_info));
	imx51_add_spi0(&spi_0_data);

	babbage_power_init();

	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();

	imx51_iim_register_fec_ethaddr();
	imx51_add_fec(&fec_info);
	imx51_add_mmc0(NULL);
	imx51_add_mmc1(NULL);

	armlinux_set_bootparams((void *)0x90000100);
	armlinux_set_architecture(MACH_TYPE_MX51_BABBAGE);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/disk0",
		BBU_HANDLER_FLAG_DEFAULT, dcd_entry, sizeof(dcd_entry), 0);

	return 0;
}

device_initcall(f3s_devices_init);

static int f3s_part_init(void)
{
	devfs_add_partition("disk0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
late_initcall(f3s_part_init);

static int f3s_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(f3s_pads, ARRAY_SIZE(f3s_pads));

	imx51_add_uart0();

	return 0;
}

console_initcall(f3s_console_init);

