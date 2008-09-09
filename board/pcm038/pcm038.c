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
#include <nand.h>
#include <spi/spi.h>
#include <asm/io.h>
#include <asm/arch/imx-nand.h>

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

#if 0
static struct device_d sram_dev = {
	.name     = "ram",
	.id       = "sram0",

	.map_base = 0xc8000000,
	.size     = 512 * 1024, /* Can be up to 2MiB */

	.type     = DEVICE_TYPE_DRAM,
};
#endif

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

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
};

static struct device_d nand_dev = {
	.name     = "imx_nand",
	.map_base = 0xd8000000,
	.platform_data	= &nand_info,
};

static int pcm038_devices_init(void)
{
	int i;
	struct device_d *nand, *dev;
	char *envdev = "no";

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

	/* configure 16 bit nor flash on cs0 */
	CS0U = 0x0000CC03;
	CS0L = 0xa0330D01;
	CS0A = 0x00220800;

	/* configure SRAM on cs1 */
	CS1U = 0x0000d843;
	CS1L = 0x22252521;
	CS1A = 0x22220a00;

	/* configure SJA1000 on cs4 */
	CS4U = 0x0000DCF6;
	CS4L = 0x444A0301;
	CS4A = 0x44443302;

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	register_device(&cfi_dev);
	register_device(&nand_dev);
	register_device(&sdram_dev);
#if 0
	register_device(&sram_dev);
#endif
	PCCR0 |= PCCR0_CSPI1_EN;
	PCCR1 |= PCCR1_PERCLK2_EN;

	spi_register_board_info(pcm038_spi_board_info, ARRAY_SIZE(pcm038_spi_board_info));
	register_device(&spi_dev);

	switch ((GPCR & GPCR_BOOT_MASK) >> GPCR_BOOT_SHIFT) {
	case GPCR_BOOT_8BIT_NAND_2k:
	case GPCR_BOOT_16BIT_NAND_2k:
	case GPCR_BOOT_16BIT_NAND_512:
	case GPCR_BOOT_8BIT_NAND_512:
		nand = get_device_by_path("/dev/nand0");
		if (!nand)
			break;
		dev = dev_add_partition(nand, 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
		if (!dev)
			break;
		dev_add_bb_dev(dev, "self0");

		dev = dev_add_partition(nand, 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
		if (!dev)
			break;
		dev_add_bb_dev(dev, "env0");
		envdev = "NAND";
		break;
	default:
		dev_add_partition(&cfi_dev, 0x00000, 0x40000, PARTITION_FIXED, "self");
		dev_add_partition(&cfi_dev, 0x40000, 0x20000, PARTITION_FIXED, "env");
		dev_protect(&cfi_dev, 0x40000, 0, 1);
		envdev = "NOR";
	}

	printf("Using environment in %s Flash\n", envdev);

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
#ifdef CONFIG_DRIVER_SPI_MC13783
	int i = 0;
	int ret;

	ret = pmic_power();
	if (ret)
		goto out;

	MPCTL0 = PLL_PCTL_PD(0) |
		PLL_PCTL_MFD(51) |
		PLL_PCTL_MFI(7) |
		PLL_PCTL_MFN(35);

	CSCR |= CSCR_MPLL_RESTART;

	while (i++ < 1000) {
		while (CCSR & CCSR_32K_SR);
		while (!(CCSR & CCSR_32K_SR));
	}

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

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024, 512, 16384);
}
#endif

static int pll_init(void)
{
	int i = 0;

	/*
	 * pll clock initialization - see section 3.4.3 of the i.MX27 manual
	 */
	MPCTL0 = PLL_PCTL_PD(1) |
		 PLL_PCTL_MFD(51) |
		 PLL_PCTL_MFI(7) |
		 PLL_PCTL_MFN(35); /* MPLL = 2 * 26 * 3.83654 MHz = 199.5 MHz */

	SPCTL0 = PLL_PCTL_PD(1) |
		 PLL_PCTL_MFD(12) |
		 PLL_PCTL_MFI(9) |
		 PLL_PCTL_MFN(3); /* SPLL = 2 * 26 * 4.61538 MHz = 240 MHz */

	/*
	 * ARM clock = (399 MHz / 2) / (ARM divider = 1) = 200 MHz
	 * AHB clock = (399 MHz / 3) / (AHB divider = 2) = 66.5 MHz
	 * System clock (HCLK) = 133 MHz
	 */
#define CSCR_VAL CSCR_USB_DIV(3) |	\
		 CSCR_SD_CNT(3) |	\
		 CSCR_MSHC_SEL |	\
		 CSCR_H264_SEL |	\
		 CSCR_SSI1_SEL |	\
		 CSCR_SSI2_SEL |	\
		 CSCR_MCU_SEL |		\
		 CSCR_SP_SEL |		\
		 CSCR_ARM_SRC_MPLL |	\
		 CSCR_AHB_DIV(1) |	\
		 CSCR_ARM_DIV(0) |	\
		 CSCR_FPM_EN |		\
		 CSCR_SPEN |		\
		 CSCR_MPEN

	CSCR = CSCR_VAL | CSCR_MPLL_RESTART | CSCR_SPLL_RESTART;

	while (i++ < 1000) {
		while (CCSR & CCSR_32K_SR);
		while (!(CCSR & CCSR_32K_SR));
	}

	/* clock gating enable */
	GPCR = 0x00050f08;

	/* peripheral clock divider */
	PCDR0 = 0x130410c3;	/* FIXME                            */
	PCDR1 = 0x09030908;	/* PERDIV1=08 @133 MHz              */

	return 0;
}

core_initcall(pll_init);

