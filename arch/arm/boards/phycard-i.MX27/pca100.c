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
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <asm/io.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <gpio.h>
#include <asm/mmu.h>
#include <usb/isp1504.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

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

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
	.phy_addr = 1,
};

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
	.flash_bbt = 1,
};

#ifdef CONFIG_USB
static struct device_d usbotg_dev = {
	.id	  = -1,
	.name     = "ehci",
	.map_base = IMX_OTG_BASE,
	.size     = 0x200,
};

static struct device_d usbh2_dev = {
	.id	  = -1,
	.name     = "ehci",
	.map_base = IMX_OTG_BASE + 0x400,
	.size     = 0x200,
};

static void pca100_usb_register(void)
{
	mdelay(10);

	gpio_direction_output(GPIO_PORTB + 24, 0);
	gpio_direction_output(GPIO_PORTB + 23, 0);

	mdelay(10);

	isp1504_set_vbus_power((void *)(IMX_OTG_BASE + 0x170), 1);
	register_device(&usbotg_dev);
	isp1504_set_vbus_power((void *)(IMX_OTG_BASE + 0x570), 1);
	register_device(&usbh2_dev);
}
#endif

#ifdef CONFIG_MMU
static void pca100_mmu_init(void)
{
	mmu_init();

	arm_create_section(0xa0000000, 0xa0000000, 128, PMD_SECT_DEF_CACHED);
	arm_create_section(0xb0000000, 0xa0000000, 128, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

	mmu_enable();
}
#else
static void pca100_mmu_init(void)
{
}
#endif

static void pca100_usb_init(void)
{
	u32 reg;

	reg = readl(IMX_OTG_BASE + 0x600);
	reg &= ~((3 << 21) | 1);
	reg |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 11) | (1 << 20);
	writel(reg, IMX_OTG_BASE + 0x600);

	/*
	 * switch usbotg and usbh2 to ulpi mode. Do this *before*
	 * the iomux setup to prevent funny hardware bugs from
	 * triggering. Also, do this even when USB support is
	 * disabled to give Linux USB support a good start.
	 */
	reg = readl(IMX_OTG_BASE + 0x584);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, IMX_OTG_BASE + 0x584);

	reg = readl(IMX_OTG_BASE + 0x184);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, IMX_OTG_BASE + 0x184);

	/* disable the usb phys */
	imx_gpio_mode((GPIO_PORTB | 23) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 23, 1);
	imx_gpio_mode((GPIO_PORTB | 24) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 24, 1);
}

static int pca100_devices_init(void)
{
	int i;
	struct device_d *nand;

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
		PD26_PF_CSPI1_SS2,
		PD27_PF_CSPI1_SS1,
		PD28_PF_CSPI1_SS0,
		PD29_PF_CSPI1_SCLK,
		PD30_PF_CSPI1_MISO,
		PD31_PF_CSPI1_MOSI,
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
		/* SDHC */
		PB4_PF_SD2_D0,
		PB5_PF_SD2_D1,
		PB6_PF_SD2_D2,
		PB7_PF_SD2_D3,
		PB8_PF_SD2_CMD,
		PB9_PF_SD2_CLK,
		PC7_PF_USBOTG_DATA5,
		PC8_PF_USBOTG_DATA6,
		PC9_PF_USBOTG_DATA0,
		PC10_PF_USBOTG_DATA2,
		PC11_PF_USBOTG_DATA1,
		PC12_PF_USBOTG_DATA4,
		PC13_PF_USBOTG_DATA3,
		PE0_PF_USBOTG_NXT,
		PE1_PF_USBOTG_STP,
		PE2_PF_USBOTG_DIR,
		PE24_PF_USBOTG_CLK,
		PE25_PF_USBOTG_DATA7,
	};

	PCCR0 |= PCCR0_SDHC2_EN;

	pca100_usb_init();

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	imx27_add_nand(&nand_info);
	register_device(&sdram_dev);
	imx27_add_fec(&fec_info);
	imx27_add_mmc0(NULL);

	PCCR1 |= PCCR1_PERCLK2_EN;

#ifdef CONFIG_USB
	pca100_usb_register();
#endif

	nand = get_device_by_name("nand0");
	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(2149);

	return 0;
}

device_initcall(pca100_devices_init);

static struct device_d pca100_serial_device = {
	.id	  = -1,
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
};

static int pca100_console_init(void)
{
	pca100_mmu_init();
	register_device(&pca100_serial_device);
	return 0;
}

console_initcall(pca100_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif

