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
#define pr_fmt(fmt) "pcm038: " fmt

#include <common.h>
#include <bootsource.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx27-regs.h>
#include <fec.h>
#include <sizes.h>
#include <notifier.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <nand.h>
#include <spi/spi.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <mach/weim.h>
#include <mach/imxfb.h>
#include <i2c/i2c.h>
#include <mach/spi.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>
#include <mach/iim.h>
#include <mfd/mc13xxx.h>
#include <mach/generic.h>

#include "pll.h"

#define PCM038_GPIO_PMIC_IRQ	(GPIO_PORTB + 23)
#define PCM038_GPIO_FEC_RST	(GPIO_PORTC + 30)
#define PCM970_GPIO_SPI_CS1	(GPIO_PORTD + 27)
#define PCM038_GPIO_SPI_CS0	(GPIO_PORTD + 28)
#define PCM038_GPIO_OTG_STP	(GPIO_PORTE + 1)

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = 1,
};

static int pcm038_spi_cs[] = {
	PCM038_GPIO_SPI_CS0,
#ifdef CONFIG_MACH_PCM970_BASEBOARD
	PCM970_GPIO_SPI_CS1,
#endif
};

static struct spi_imx_master pcm038_spi_0_data = {
	.chipselect = pcm038_spi_cs,
	.num_chipselect = ARRAY_SIZE(pcm038_spi_cs),
};

static struct spi_board_info pcm038_spi_board_info[] = {
	{
		.name = "mc13783",
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
	.num_modes = 1,
	.pwmr	= 0x00A903FF,
	.lscr1	= 0x00120300,
	.dmacr	= 0x00020010,
};

/**
 * The spctl0 register is a beast: Seems you can read it
 * only one times without writing it again.
 */
static inline uint32_t get_pll_spctl10(void)
{
	uint32_t reg;

	reg = readl(MX27_CCM_BASE_ADDR + MX27_SPCTL0);
	writel(reg, MX27_CCM_BASE_ADDR + MX27_SPCTL0);

	return reg;
}

/**
 * If the PLL settings are in place switch the CPU core frequency to the max. value
 */
static int pcm038_power_init(void)
{
	uint32_t spctl0 = get_pll_spctl10();
	struct mc13xxx *mc13xxx = mc13xxx_get();

	/* PLL registers already set to their final values? */
	if (spctl0 == SPCTL0_VAL &&
	    readl(MX27_CCM_BASE_ADDR + MX27_MPCTL0) == MPCTL0_VAL) {
		console_flush();
		if (mc13xxx) {
			mc13xxx_reg_write(mc13xxx, MC13783_REG_SWITCHERS(0),
				MC13783_SWX_VOLTAGE(MC13783_SWX_VOLTAGE_1_450) |
				MC13783_SWX_VOLTAGE_DVS(MC13783_SWX_VOLTAGE_1_450) |
				MC13783_SWX_VOLTAGE_STANDBY(MC13783_SWX_VOLTAGE_1_450));

			mc13xxx_reg_write(mc13xxx, MC13783_REG_SWITCHERS(4),
				MC13783_SW1A_MODE(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1A_MODE_STANDBY(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1A_SOFTSTART |
				MC13783_SW1B_MODE(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1B_MODE_STANDBY(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1B_SOFTSTART |
				MC13783_SW_PLL_FACTOR(32));

			/* Setup VMMC voltage */
			if (IS_ENABLED(CONFIG_MCI_IMX)) {
				u32 val;

				mc13xxx_reg_read(mc13xxx, MC13783_REG_REG_SETTING(1), &val);
				/* VMMC1 = 3.00 V */
				val &= ~(7 << 6);
				val |= 6 << 6;
				mc13xxx_reg_write(mc13xxx, MC13783_REG_REG_SETTING(1), val);

				mc13xxx_reg_read(mc13xxx, MC13783_REG_REG_MODE(1), &val);
				/* Enable VMMC1 */
				val |= 1 << 18;
				mc13xxx_reg_write(mc13xxx, MC13783_REG_REG_MODE(1), val);
			}

			/* wait for required power level to run the CPU at 400 MHz */
			udelay(100000);
			writel(CSCR_VAL_FINAL, MX27_CCM_BASE_ADDR + MX27_CSCR);
			writel(0x130410c3, MX27_CCM_BASE_ADDR + MX27_PCDR0);
			writel(0x09030911, MX27_CCM_BASE_ADDR + MX27_PCDR1);

			/* Clocks have changed. Notify clients */
			clock_notifier_call_chain();
		} else {
			pr_err("Failed to initialize PMIC. Will continue with low CPU speed\n");
		}
	}

	/* clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR);

	return 0;
}

struct imxusb_platformdata pcm038_otg_pdata = {
	.mode	= IMX_USB_MODE_DEVICE,
	.flags	= MXC_EHCI_MODE_ULPI | MXC_EHCI_INTERFACE_DIFF_UNI,
};

static int pcm038_devices_init(void)
{
	int i;
	u64 uid = 0;
	char *envdev;
	long sram_size;

	unsigned int mode[] = {
		/* FEC */
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
		/* UART1 */
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		/* CSPI1 */
		PD25_PF_CSPI1_RDY,
		PD29_PF_CSPI1_SCLK,
		PD30_PF_CSPI1_MISO,
		PD31_PF_CSPI1_MOSI,
		/* Display */
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
		/* USB OTG */
		PC7_PF_USBOTG_DATA5,
		PC8_PF_USBOTG_DATA6,
		PC9_PF_USBOTG_DATA0,
		PC10_PF_USBOTG_DATA2,
		PC11_PF_USBOTG_DATA1,
		PC12_PF_USBOTG_DATA4,
		PC13_PF_USBOTG_DATA3,
		PE0_PF_USBOTG_NXT,
		PCM038_GPIO_OTG_STP | GPIO_GPIO | GPIO_OUT,
		PE2_PF_USBOTG_DIR,
		PE24_PF_USBOTG_CLK,
		PE25_PF_USBOTG_DATA7,
		/* I2C1 */
		PD17_PF_I2C_DATA | GPIO_PUEN,
		PD18_PF_I2C_CLK,
		/* I2C2 */
		PC5_PF_I2C2_SDA,
		PC6_PF_I2C2_SCL,
		/* Misc */
		PCM038_GPIO_FEC_RST | GPIO_GPIO | GPIO_OUT,
		PCM038_GPIO_SPI_CS0 | GPIO_GPIO | GPIO_OUT,
#ifdef CONFIG_MACH_PCM970_BASEBOARD
		PCM970_GPIO_SPI_CS1 | GPIO_GPIO | GPIO_OUT,
#endif
		PCM038_GPIO_PMIC_IRQ | GPIO_GPIO | GPIO_IN,
	};

	/* configure 16 bit nor flash on cs0 */
	imx27_setup_weimcs(0, 0x22C2CF00, 0x75000D01, 0x00000900);

	/* configure SRAM on cs1 */
	imx27_setup_weimcs(1, 0x0000d843, 0x22252521, 0x22220a00);

	/* SRAM can be up to 2MiB */
	sram_size = get_ram_size((ulong *)MX27_CS1_BASE_ADDR, SZ_2M);
	if (sram_size)
		add_mem_device("ram1", MX27_CS1_BASE_ADDR, sram_size,
			       IORESOURCE_MEM_WRITEABLE);

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	spi_register_board_info(pcm038_spi_board_info, ARRAY_SIZE(pcm038_spi_board_info));
	imx27_add_spi0(&pcm038_spi_0_data);

	pcm038_power_init();

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC0000000, 32 * 1024 * 1024, 0);
	imx27_add_nand(&nand_info);
	imx27_add_fb(&pcm038_fb_data);

	imx27_add_i2c0(NULL);
	imx27_add_i2c1(NULL);

	/* Register the fec device after the PLL re-initialisation
	 * as the fec depends on the (now higher) ipg clock
	 */
	gpio_set_value(PCM038_GPIO_FEC_RST, 1);
	imx27_add_fec(&fec_info);

	/* Apply delay for STP line to stop ULPI */
	gpio_direction_output(PCM038_GPIO_OTG_STP, 1);
	mdelay(1);
	imx_gpio_mode(PE1_PF_USBOTG_STP);

	if (IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC))
		imx27_add_usbotg(&pcm038_otg_pdata);

	switch (bootsource_get()) {
	case BOOTSOURCE_NAND:
		devfs_add_partition("nand0", 0, SZ_512K,
				    DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", SZ_512K, SZ_128K,
				    DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		envdev = "NAND";
		break;
	default:
		devfs_add_partition("nor0", 0, SZ_512K,
				    DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("nor0", SZ_512K, SZ_128K,
				    DEVFS_PARTITION_FIXED, "env0");
		protect_file("/dev/env0", 1);
		envdev = "NOR";
	}

	pr_notice("Using environment in %s Flash\n", envdev);

	if (imx_iim_read(1, 0, &uid, 6) == 6)
		armlinux_set_serial(uid);
	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_PCM038);

	return 0;
}

device_initcall(pcm038_devices_init);

static int pcm038_console_init(void)
{
	barebox_set_model("Phytec phyCORE-i.MX27");
	barebox_set_hostname("phycore-imx27");

	imx27_add_uart0();

	return 0;
}

console_initcall(pcm038_console_init);
