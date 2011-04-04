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
#include <init.h>
#include <environment.h>
#include <mach/imx-regs.h>
#include <fec.h>
#include <notifier.h>
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/pmic.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <command.h>
#include <spi/spi.h>
#include <asm/io.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <mach/imxfb.h>
#include <asm/mmu.h>
#include <usb/isp1504.h>
#include <mach/spi.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

#include "pll.h"

static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = 0xC0000000,
	.size     = 32 * 1024 * 1024,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0xa0000000,
	.size     = 128 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct memory_platform_data sram_pdata = {
	.name = "sram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0xc8000000,
	.size     = 512 * 1024, /* Can be up to 2MiB */
	.platform_data = &sram_pdata,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
	.phy_addr = 1,
};

static int pcm038_spi_cs[] = {GPIO_PORTD + 28};

static struct spi_imx_master pcm038_spi_0_data = {
	.chipselect = pcm038_spi_cs,
	.num_chipselect = ARRAY_SIZE(pcm038_spi_cs),
};

static struct spi_board_info pcm038_spi_board_info[] = {
	{
		.name = "mc13783",
		.max_speed_hz = 3000000,
		.bus_num = 0,
		.chip_select = 0,
	}
};

static struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct imx_fb_videomode imxfb_mode = {
	.mode = {
		.name		= "Sharp-LQ035Q7",
		.refresh	= 60,
		.xres		= 240,
		.yres		= 320,
		.pixclock	= 188679, /* in ps (5.3MHz) */
		.hsync_len	= 7,
		.left_margin	= 5,
		.right_margin	= 16,
		.vsync_len	= 1,
		.upper_margin	= 7,
		.lower_margin	= 9,
	},
	/*
	 * - HSYNC active high
	 * - VSYNC active high
	 * - clk notenabled while idle
	 * - clock not inverted
	 * - data not inverted
	 * - data enable low active
	 * - enable sharp mode
	 */
	.pcr		= 0xF00080C0,
	.bpp		= 16,
};

static struct imx_fb_platform_data pcm038_fb_data = {
	.mode	= &imxfb_mode,
	.pwmr	= 0x00A903FF,
	.lscr1	= 0x00120300,
	.dmacr	= 0x00020010,
};

#ifdef CONFIG_USB
static struct device_d usbh2_dev = {
	.id	  = -1,
	.name     = "ehci",
	.map_base = IMX_OTG_BASE + 0x400,
	.size     = 0x200,
};

static void pcm038_usbh_init(void)
{
	uint32_t temp;

	temp = readl(IMX_OTG_BASE + 0x600);
	temp &= ~((3 << 21) | 1);
	temp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20);
	writel(temp, IMX_OTG_BASE + 0x600);

	temp = readl(IMX_OTG_BASE + 0x584);
	temp &= ~(3 << 30);
	temp |= 2 << 30;
	writel(temp, IMX_OTG_BASE + 0x584);

	mdelay(10);

	isp1504_set_vbus_power((void *)(IMX_OTG_BASE + 0x570), 1);
}
#endif

#ifdef CONFIG_MMU
static void pcm038_mmu_init(void)
{
	mmu_init();

	arm_create_section(0xa0000000, 0xa0000000, 128, PMD_SECT_DEF_CACHED);
	arm_create_section(0xb0000000, 0xa0000000, 128, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

	mmu_enable();
}
#else
static void pcm038_mmu_init(void)
{
}
#endif

static int pcm038_devices_init(void)
{
	int i;
	char *envdev;

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
		PD14_AOUT_FEC_RX_CLK,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		PD25_PF_CSPI1_RDY,
		GPIO_PORTD | 28 | GPIO_GPIO | GPIO_OUT,
		PD29_PF_CSPI1_SCLK,
		PD30_PF_CSPI1_MISO,
		PD31_PF_CSPI1_MOSI,
		/* display */
		PA5_PF_LSCLK,
		PA6_PF_LD0,
		PA7_PF_LD1,
		PA8_PF_LD2,
		PA9_PF_LD3,
		PA10_PF_LD4,
		PA11_PF_LD5,
		PA12_PF_LD6,
		PA13_PF_LD7,
		PA14_PF_LD8,
		PA15_PF_LD9,
		PA16_PF_LD10,
		PA17_PF_LD11,
		PA18_PF_LD12,
		PA19_PF_LD13,
		PA20_PF_LD14,
		PA21_PF_LD15,
		PA22_PF_LD16,
		PA23_PF_LD17,
		PA24_PF_REV,
		PA25_PF_CLS,
		PA26_PF_PS,
		PA27_PF_SPL_SPR,
		PA28_PF_HSYNC,
		PA29_PF_VSYNC,
		PA30_PF_CONTRAST,
		PA31_PF_OE_ACD,
		/* USB host 2 */
		PA0_PF_USBH2_CLK,
		PA1_PF_USBH2_DIR,
		PA2_PF_USBH2_DATA7,
		PA3_PF_USBH2_NXT,
		PA4_PF_USBH2_STP,
		PD19_AF_USBH2_DATA4,
		PD20_AF_USBH2_DATA3,
		PD21_AF_USBH2_DATA6,
		PD22_AF_USBH2_DATA0,
		PD23_AF_USBH2_DATA2,
		PD24_AF_USBH2_DATA1,
		PD26_AF_USBH2_DATA5,
	};

	pcm038_mmu_init();

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

	PCCR0 |= PCCR0_CSPI1_EN;
	PCCR1 |= PCCR1_PERCLK2_EN;

	gpio_direction_output(GPIO_PORTD | 28, 0);
	gpio_set_value(GPIO_PORTD | 28, 0);
	spi_register_board_info(pcm038_spi_board_info, ARRAY_SIZE(pcm038_spi_board_info));
	imx27_add_spi0(&pcm038_spi_0_data);

	register_device(&cfi_dev);
	imx27_add_nand(&nand_info);
	register_device(&sdram_dev);
	register_device(&sram_dev);
	imx27_add_fb(&pcm038_fb_data);

#ifdef CONFIG_USB
	pcm038_usbh_init();
	register_device(&usbh2_dev);
#endif

	/* Register the fec device after the PLL re-initialisation
	 * as the fec depends on the (now higher) ipg clock
	 */
	imx27_add_fec(&fec_info);

	switch ((GPCR & GPCR_BOOT_MASK) >> GPCR_BOOT_SHIFT) {
	case GPCR_BOOT_8BIT_NAND_2k:
	case GPCR_BOOT_16BIT_NAND_2k:
	case GPCR_BOOT_16BIT_NAND_512:
	case GPCR_BOOT_8BIT_NAND_512:
		devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");

		devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		envdev = "NAND";
		break;
	default:
		devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
		devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");
		protect_file("/dev/env0", 1);

		envdev = "NOR";
	}

	printf("Using environment in %s Flash\n", envdev);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_PCM038);

	return 0;
}

device_initcall(pcm038_devices_init);

static int pcm038_console_init(void)
{
	imx27_add_uart0();

	return 0;
}

console_initcall(pcm038_console_init);

/**
 * The spctl0 register is a beast: Seems you can read it
 * only one times without writing it again.
 */
static inline uint32_t get_pll_spctl10(void)
{
	uint32_t reg;

	reg = SPCTL0;
	SPCTL0 = reg;

	return reg;
}

/**
 * If the PLL settings are in place switch the CPU core frequency to the max. value
 */
static int pcm038_power_init(void)
{
	uint32_t spctl0;
	int ret;

	spctl0 = get_pll_spctl10();

	/* PLL registers already set to their final values? */
	if (spctl0 == SPCTL0_VAL && MPCTL0 == MPCTL0_VAL) {
		console_flush();
		ret = pmic_power();
		if (ret == 0) {
			/* wait for required power level to run the CPU at 400 MHz */
			udelay(100000);
			CSCR = CSCR_VAL_FINAL;
			PCDR0 = 0x130410c3;
			PCDR1 = 0x09030911;
			/* Clocks have changed. Notify clients */
			clock_notifier_call_chain();
		} else {
			printf("Failed to initialize PMIC. Will continue with low CPU speed\n");
		}
	}

	/* clock gating enable */
	GPCR = 0x00050f08;

	return 0;
}

late_initcall(pcm038_power_init);

