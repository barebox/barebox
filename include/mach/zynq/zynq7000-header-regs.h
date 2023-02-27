/*
 * (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define ZYNQ_SLCR_LOCK			0xF8000004
#define ZYNQ_SLCR_UNLOCK		0xF8000008
#define ZYNQ_ARM_PLL_CTRL		0xF8000100
#define ZYNQ_DDR_PLL_CTRL		0xF8000104
#define ZYNQ_IO_PLL_CTRL		0xF8000108
#define ZYNQ_PLL_STATUS			0xF800010C
#define ZYNQ_ARM_PLL_CFG		0xF8000110
#define ZYNQ_DDR_PLL_CFG		0xF8000114
#define ZYNQ_IO_PLL_CFG			0xF8000118
#define ZYNQ_ARM_CLK_CTRL		0xF8000120
#define ZYNQ_DDR_CLK_CTRL		0xF8000124
#define ZYNQ_DCI_CLK_CTRL		0xF8000128
#define ZYNQ_APER_CLK_CTRL		0xF800012C
#define ZYNQ_USB0_CLK_CTRL		0xF8000130
#define ZYNQ_USB1_CLK_CTRL		0xF8000134
#define ZYNQ_GEM0_RCLK_CTRL		0xF8000138
#define ZYNQ_GEM1_RCLK_CTRL		0xF800013C
#define ZYNQ_GEM0_CLK_CTRL		0xF8000140
#define ZYNQ_GEM1_CLK_CTRL		0xF8000144
#define ZYNQ_SMC_CLK_CTRL		0xF8000148
#define ZYNQ_LQSPI_CLK_CTRL		0xF800014C
#define ZYNQ_SDIO_CLK_CTRL		0xF8000150
#define ZYNQ_UART_CLK_CTRL		0xF8000154
#define ZYNQ_SPI_CLK_CTRL		0xF8000158
#define ZYNQ_CAN_CLK_CTRL		0xF800015C
#define ZYNQ_CAN_MIOCLK_CTRL	0xF8000160
#define ZYNQ_DBG_CLK_CTRL		0xF8000164
#define ZYNQ_PCAP_CLK_CTRL		0xF8000168
#define ZYNQ_TOPSW_CLK_CTRL		0xF800016C
#define ZYNQ_FPGA0_CLK_CTRL		0xF8000170
#define ZYNQ_FPGA1_CLK_CTRL		0xF8000180
#define ZYNQ_FPGA2_CLK_CTRL		0xF8000190
#define ZYNQ_FPGA3_CLK_CTRL		0xF80001A0
#define ZYNQ_CLK_621_TRUE		0xF80001C4
