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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <net.h>
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <asm/arch/imx-regs.h>
#include <fec.h>
#include <asm/arch/gpio.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <asm/arch/pmic.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <spi/spi.h>
#include <asm/io.h>

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0xC0000000,
	.size     = 32 * 1024 * 1024,
};

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0xa0000000,
	.size     = 128 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
};

static struct device_d fec_dev = {
	.name     = "fec_imx27",
	.id       = "eth0",
	.map_base = 0x1002b000,
	.platform_data	= &fec_info,
	.type     = DEVICE_TYPE_ETHER,
};

#if defined CONFIG_DRIVER_SPI_IMX && defined DRIVER_SPI_MC13783
static struct device_d spi_dev = {
	.name     = "imx_spi",
	.id       = "spi0",
	.map_base = 0x1000e000,
};

static struct spi_board_info pcm038_spi_board_info[] = {
	{
		.name = "mc13783",
		.max_speed_hz = 3000000,
		.bus_num = 0,
		.chip_select = 0,
	}
};
#endif

static int pcm038_devices_init(void)
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
		PD14_AOUT_FEC_CLR,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		PD25_PF_CSPI1_RDY,
		PD26_PF_CSPI1_SS2,
		PD27_PF_CSPI1_SS1,
		PD28_PF_CSPI1_SS0,
		PD29_PF_CSPI1_SCLK,
		PD30_PF_CSPI1_MISO,
		PD31_PF_CSPI1_MOSI,
	};

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	register_device(&cfi_dev);
	register_device(&sdram_dev);

	PCCR0 |= PCCR0_CSPI1_EN;
	PCCR1 |= PCCR1_PERCLK2_EN;

#if defined CONFIG_DRIVER_SPI_IMX && defined DRIVER_SPI_MC13783
	spi_register_board_info(pcm038_spi_board_info, ARRAY_SIZE(pcm038_spi_board_info));
	register_device(&spi_dev);
#endif

#ifdef CONDFIG_PARTITION
	dev_add_partition(&cfi_dev, 0x00000, 0x20000, PARTITION_FIXED, "self");
	dev_add_partition(&cfi_dev, 0x20000, 0x20000, PARTITION_FIXED, "env");
#endif
	dev_protect(&cfi_dev, 0x20000, 0, 1);

	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_PCM038);

	return 0;
}

device_initcall(pcm038_devices_init);

static struct device_d pcm038_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int pcm038_console_init(void)
{
	register_device(&pcm038_serial_device);
	return 0;
}

console_initcall(pcm038_console_init);

static int pcm038_power_init(void)
{
#if defined CONFIG_DRIVER_SPI_IMX && defined DRIVER_SPI_MC13783
	volatile int i = 0;
	int ret;

	ret = pmic_power();
	if (ret)
		goto out;

	MPCTL0 = PLL_PCTL_PD(0) |
		PLL_PCTL_MFD(51) |
		PLL_PCTL_MFI(7) |
		PLL_PCTL_MFN(35);

	CSCR |= CSCR_MPLL_RESTART;

	/* We need a delay here. We can't use udelay because
	 * the PLL is not running. Do not remove the volatile
	 * above because otherwise the compiler will optimize the loop
	 * away.
	 */
	while(i++ < 10000);

	CSCR = CSCR_USB_DIV(3) |	\
		 CSCR_SD_CNT(3) |	\
		 CSCR_MSHC_SEL |	\
		 CSCR_H264_SEL |	\
		 CSCR_SSI1_SEL |	\
		 CSCR_SSI2_SEL |	\
		 CSCR_MCU_SEL |		\
		 CSCR_SP_SEL |		\
		 CSCR_ARM_SRC_MPLL |	\
		 CSCR_ARM_DIV(0) | 	\
		 CSCR_AHB_DIV(1) |	\
		 CSCR_FPM_EN |		\
		 CSCR_MPEN;

	PCDR1 = 0x09030911;

out:
#else
#warning no pmic support enabled. your pcm038 will run on low speed
#endif

	/* Register the fec device after the PLL re-initialisation
	 * as the fec depends on the (now higher) ipg clock
	 */
	register_device(&fec_dev);

	return 0;
}

late_initcall(pcm038_power_init);

