/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2012 Christian Kapeller, <christian.kapeller@cmotion.eu>
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
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <io.h>
#include <asm/mmu.h>
#include <mach/imx5.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/iim.h>


#define STK5_MX51_PAD_DISPB2_SER_RS__GPIO3_8 \
    IOMUX_PAD(0x6C8, 0x2C8, 4, 0x994, 1, PAD_CTL_PKE | PAD_CTL_PUE)

#define STK5_MX51_PAD_DISPB2_SER_DIO__GPIO3_6 \
    IOMUX_PAD(0x6c0, 0x2c0, 4, 0x098c, 1, 0)

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
	.flash_bbt = 1,
};

struct gpio_led tx51_leds[] = {
	{
		.led = { .name = "GPIO-LED", },
		.gpio = IMX_GPIO_NR(4,10),
		.active_low = 0,
	},
};

static iomux_v3_cfg_t tx51_pads[] = {
	/*UART1*/
	MX51_PAD_UART1_RXD__UART1_RXD,
	MX51_PAD_UART1_TXD__UART1_TXD,
	MX51_PAD_UART1_CTS__UART1_CTS,
	MX51_PAD_UART1_RTS__UART1_RTS,

	/* (e)CSPI */
	MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
	MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
	MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,
	MX51_PAD_CSPI1_RDY__ECSPI1_RDY,

	/* (e)CSPI chip select lines */
	MX51_PAD_CSPI1_SS0__GPIO4_24,
	MX51_PAD_CSPI1_SS1__GPIO4_25,

#ifdef CONFIG_MCI_IMX_ESDHC
	/* eSDHC 1 */
	MX51_PAD_SD1_CMD__SD1_CMD,
	MX51_PAD_SD1_CLK__SD1_CLK,
	MX51_PAD_SD1_DATA0__SD1_DATA0,
	MX51_PAD_SD1_DATA1__SD1_DATA1,
	MX51_PAD_SD1_DATA2__SD1_DATA2,
	MX51_PAD_SD1_DATA3__SD1_DATA3,

	/* SD1 card detect */
	STK5_MX51_PAD_DISPB2_SER_RS__GPIO3_8,

	/* eSDHC 2 */
	MX51_PAD_SD2_CMD__SD2_CMD,
	MX51_PAD_SD2_CLK__SD2_CLK,
	MX51_PAD_SD2_DATA0__SD2_DATA0,
	MX51_PAD_SD2_DATA1__SD2_DATA1,
	MX51_PAD_SD2_DATA2__SD2_DATA2,
	MX51_PAD_SD2_DATA3__SD2_DATA3,

	/* SD2 card detect */
	STK5_MX51_PAD_DISPB2_SER_DIO__GPIO3_6,
#endif

	/* SW controlled LED on STK5 baseboard */
	MX51_PAD_CSI2_D13__GPIO4_10,

	/* unuseable pads configured as GPIO */
	MX51_PAD_GPIO1_1__GPIO1_1,
	MX51_PAD_GPIO1_0__GPIO1_0,
};

static int spi_0_cs[] = {
	IMX_GPIO_NR(4, 24),
	IMX_GPIO_NR(4, 25),
};

static struct spi_imx_master tx51_spi_0_data = {
	.chipselect = spi_0_cs,
	.num_chipselect = ARRAY_SIZE(spi_0_cs),
};

static const struct spi_board_info mx51_tx51_spi_board_info[] = {};

static struct tx51_fec_gpio_setup {
	iomux_v3_cfg_t pad;
	unsigned group:4,
		shift:5,
		level:1;
} tx51_fec_gpios[] = {
	{ MX51_PAD_EIM_A20__GPIO2_14,	2, 14, 0 }, /* PHY reset */
	{ MX51_PAD_GPIO1_3__GPIO1_3,	1, 3, 0 }, /* PHY power enable */
	{ MX51_PAD_NANDF_CS3__GPIO3_19,	3, 19, 0 }, /* MDC */
	{ MX51_PAD_EIM_EB2__GPIO2_22,	2, 22, 0 }, /* MDIO */
	{ MX51_PAD_NANDF_RB3__GPIO3_11,	3, 11, 0 }, /* RX_CLK */
	{ MX51_PAD_NANDF_D11__GPIO3_29,	3, 29, 0 }, /* RX_DV */
	{ MX51_PAD_NANDF_D9__GPIO3_31,	3, 31, 1 }, /* RXD0/Mode0 */
	{ MX51_PAD_EIM_EB3__GPIO2_23,	2, 23, 1 }, /* RXD1/Mode1 */
	{ MX51_PAD_EIM_CS2__GPIO2_27,	2, 27, 1 }, /* RXD2/Mode2 */
	{ MX51_PAD_EIM_CS3__GPIO2_28,	2, 28, 1 }, /* RXD3/nINTSEL */
	{ MX51_PAD_EIM_CS4__GPIO2_29,	2, 29, 0 }, /* RX_ER/RXD4 */
	{ MX51_PAD_NANDF_RDY_INT__GPIO3_24,	3, 24, 0 }, /* TX_CLK */
	{ MX51_PAD_NANDF_CS7__GPIO3_23,	3, 23, 0 }, /* TX_EN */
	{ MX51_PAD_NANDF_D8__GPIO4_0,	4, 0, 0 }, /* TXD0 */
	{ MX51_PAD_NANDF_CS4__GPIO3_20,	3, 20, 0 }, /* TXD1 */
	{ MX51_PAD_NANDF_CS5__GPIO3_21,	3, 21, 0 }, /* TXD2 */
	{ MX51_PAD_NANDF_CS6__GPIO3_22,	3, 22, 0 }, /* TXD3 */
	{ MX51_PAD_NANDF_RB2__GPIO3_10,	3, 10, 0 }, /* COL/RMII/CRSDV */
	{ MX51_PAD_EIM_CS5__GPIO2_30,	2, 30, 0 }, /* CRS */
	{ MX51_PAD_NANDF_CS2__GPIO3_18,	3, 18, 0 }, /* nINT/TX_ER/TXD4 */
};

static iomux_v3_cfg_t tx51_fec_pads[] = {
	MX51_PAD_NANDF_CS2__FEC_TX_ER,
	MX51_PAD_NANDF_RDY_INT__FEC_TX_CLK,
	MX51_PAD_NANDF_CS3__FEC_MDC,
	MX51_PAD_NANDF_CS4__FEC_TDATA1,
	MX51_PAD_NANDF_CS5__FEC_TDATA2,
	MX51_PAD_NANDF_CS6__FEC_TDATA3,
	MX51_PAD_NANDF_CS7__FEC_TX_EN,
	MX51_PAD_NANDF_RB2__FEC_COL,
	MX51_PAD_NANDF_RB3__FEC_RX_CLK,
	MX51_PAD_NANDF_D8__FEC_TDATA0,
	MX51_PAD_NANDF_D9__FEC_RDATA0,
	MX51_PAD_NANDF_D11__FEC_RX_DV,
	MX51_PAD_EIM_EB2__FEC_MDIO,
	MX51_PAD_EIM_EB3__FEC_RDATA1,
	MX51_PAD_EIM_CS2__FEC_RDATA2,
	MX51_PAD_EIM_CS3__FEC_RDATA3,
	MX51_PAD_EIM_CS4__FEC_RX_ER,
	MX51_PAD_EIM_CS5__FEC_CRS,
};

#define TX51_FEC_PHY_RST	IMX_GPIO_NR(2, 14)
#define TX51_FEC_PHY_PWR	IMX_GPIO_NR(1, 3)
#define TX51_FEC_PHY_INT	IMX_GPIO_NR(3, 18)

static inline void tx51_fec_init(void)
{
	int i;

	/* Configure LAN8700 pads as GPIO and set up
	 * necessary strap options for PHY
	 */
	for (i = 0; i < ARRAY_SIZE(tx51_fec_gpios); i++) {
		struct tx51_fec_gpio_setup *gs = &tx51_fec_gpios[i];

		gpio_direction_output(IMX_GPIO_NR(gs->group, gs->shift ), gs->level);
		mxc_iomux_v3_setup_pad(gs->pad);
	}

	/*
	 *Turn on phy power, leave in reset state
	 */
	gpio_set_value(TX51_FEC_PHY_PWR, 1);

	/*
	 * Wait some time to let the phy activate the internal regulator
	 */
	mdelay(10);

	/*
	 * Deassert reset, phy latches the rest of bootstrap pins
	 */
	gpio_set_value(TX51_FEC_PHY_RST, 1);

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
	mxc_iomux_v3_setup_multiple_pads(tx51_fec_pads,
			ARRAY_SIZE(tx51_fec_pads));
}

static void tx51_leds_init(void)
{
	int i;

	for (i = 0 ; i < ARRAY_SIZE(tx51_leds) ; i++)
		led_gpio_register(&tx51_leds[i]);
}

static int tx51_devices_init(void)
{
#ifdef CONFIG_MCI_IMX_ESDHC
	imx51_add_mmc0(NULL);
	imx51_add_mmc1(NULL);
#endif

	imx51_add_nand(&nand_info);

	spi_register_board_info(mx51_tx51_spi_board_info,
		ARRAY_SIZE(mx51_tx51_spi_board_info));
	imx51_add_spi0(&tx51_spi_0_data);

	imx51_iim_register_fec_ethaddr();
	tx51_fec_init();
	imx51_add_fec(&fec_info);

	tx51_leds_init();

	//Linux Parameters
	armlinux_set_bootparams((void *)MX51_CSD0_BASE_ADDR + 0x100);
	armlinux_set_architecture(MACH_TYPE_TX51);

	return 0;
}
device_initcall(tx51_devices_init);

static int tx51_part_init(void)
{
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x80000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	return 0;
}
late_initcall(tx51_part_init);

static int tx51_console_init(void)
{
	imx51_init_lowlevel(800);
	mxc_iomux_v3_setup_multiple_pads(tx51_pads, ARRAY_SIZE(tx51_pads));

	barebox_set_model("Ka-Ro TX51");
	barebox_set_hostname("tx51");

	imx51_add_uart0();

	return 0;
}
console_initcall(tx51_console_init);
