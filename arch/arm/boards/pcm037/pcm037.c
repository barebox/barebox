/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * Board support for Phytec's, i.MX31 based CPU card, called: PCM037
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <fs.h>
#include <gpio.h>
#include <environment.h>
#include <usb/ulpi.h>
#include <mach/imx31-regs.h>
#include <mach/iomux-mx31.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <mach/weim.h>
#include <io.h>
#include <smc911x.h>
#include <asm/mmu.h>
#include <partition.h>
#include <generated/mach-types.h>
#include <asm/barebox-arm.h>
#include <mach/imx-nand.h>
#include <mach/devices-imx31.h>

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
	.flash_bbt = 1,
};

#ifdef CONFIG_USB
static void pcm037_usb_init(void)
{
	u32 tmp;

	/* enable clock */
	tmp = readl(0x53f80000);
	tmp |= (1 << 9);
	writel(tmp, 0x53f80000);

	/* Host 1 */
	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x600);
	tmp &= ~((3 << 21) | 1);
	tmp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 11) | (1 << 20);
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x600);

	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x184);
	tmp &= ~(3 << 30);
	tmp |= 2 << 30;
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x184);

	imx_iomux_mode(MX31_PIN_USBOTG_DATA0__USBOTG_DATA0);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA1__USBOTG_DATA1);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA2__USBOTG_DATA2);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA3__USBOTG_DATA3);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA4__USBOTG_DATA4);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA5__USBOTG_DATA5);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA6__USBOTG_DATA6);
	imx_iomux_mode(MX31_PIN_USBOTG_DATA7__USBOTG_DATA7);
	imx_iomux_mode(MX31_PIN_USBOTG_CLK__USBOTG_CLK);
	imx_iomux_mode(MX31_PIN_USBOTG_DIR__USBOTG_DIR);
	imx_iomux_mode(MX31_PIN_USBOTG_NXT__USBOTG_NXT);
	imx_iomux_mode(MX31_PIN_USBOTG_STP__USBOTG_STP);

	mdelay(50);
	ulpi_setup((void *)(MX31_USB_OTG_BASE_ADDR + 0x170), 1);

	/* Host 2 */
	tmp = readl(MX31_IOMUXC_GPR);
	tmp |= 1 << 11;	/* IOMUX GPR: enable USBH2 signals */
	writel(tmp, MX31_IOMUXC_GPR);

	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_CLK, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_DIR, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_NXT, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_STP, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_DATA0, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_USBH2_DATA1, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_STXD3, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_SRXD3, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_SCK3, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_SFS3, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_STXD6, IOMUX_CONFIG_FUNC));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_SRXD6, IOMUX_CONFIG_FUNC));

#define H2_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)
	imx_iomux_set_pad(MX31_PIN_USBH2_CLK, H2_PAD_CFG);
	imx_iomux_set_pad(MX31_PIN_USBH2_DIR, H2_PAD_CFG);
	imx_iomux_set_pad(MX31_PIN_USBH2_NXT, H2_PAD_CFG);
	imx_iomux_set_pad(MX31_PIN_USBH2_STP, H2_PAD_CFG);
	imx_iomux_set_pad(MX31_PIN_USBH2_DATA0, H2_PAD_CFG); /* USBH2_DATA0 */
	imx_iomux_set_pad(MX31_PIN_USBH2_DATA1, H2_PAD_CFG); /* USBH2_DATA1 */
	imx_iomux_set_pad(MX31_PIN_SRXD6, H2_PAD_CFG);	/* USBH2_DATA2 */
	imx_iomux_set_pad(MX31_PIN_STXD6, H2_PAD_CFG);	/* USBH2_DATA3 */
	imx_iomux_set_pad(MX31_PIN_SFS3, H2_PAD_CFG);	/* USBH2_DATA4 */
	imx_iomux_set_pad(MX31_PIN_SCK3, H2_PAD_CFG);	/* USBH2_DATA5 */
	imx_iomux_set_pad(MX31_PIN_SRXD3, H2_PAD_CFG);	/* USBH2_DATA6 */
	imx_iomux_set_pad(MX31_PIN_STXD3, H2_PAD_CFG);	/* USBH2_DATA7 */

	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x600);
	tmp &= ~((3 << 21) | 1);
	tmp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20);
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x600);

	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x584);
	tmp &= ~(3 << 30);
	tmp |= 2 << 30;
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x584);

	mdelay(50);
	ulpi_setup((void *)(MX31_USB_OTG_BASE_ADDR + 0x570), 1);

	/* Set to Host mode */
	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x1a8);
	writel(tmp | 0x3, MX31_USB_OTG_BASE_ADDR + 0x1a8);

}
#endif

static int pcm037_mmu_init(void)
{
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);

	return 0;
}
postmmu_initcall(pcm037_mmu_init);

static struct smc911x_plat smsc9217_pdata = {
	.flags = SMC911X_FORCE_INTERNAL_PHY,
};

static int pcm037_devices_init(void)
{
	/* CS0: Nor Flash */
	imx31_setup_weimcs(0, 0x0000cf03, 0x10000d03, 0x00720900);
	/* CS1: Network Controller */
	imx31_setup_weimcs(1, 0x0000df06, 0x444a4541, 0x44443302);
	/* CS4: SRAM */
	imx31_setup_weimcs(4, 0x0000d843, 0x22252521, 0x22220a00);
	/* CS5: SJA1000 */
	imx31_setup_weimcs(4, 0x0000DCF6, 0x444A0301, 0x44443302);

	/*
	 * Up to 32MiB NOR type flash, connected to
	 * CS line 0, data width is 16 bit
	 */
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, MX31_CS0_BASE_ADDR, 32 * 1024 * 1024, 0);

	imx31_add_mmc0(NULL);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");	/* ourself */
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");	/* environment */

	protect_file("/dev/env0", 1);

	/*
	 * up to 2MiB static RAM type memory, connected
	 * to CS4, data width is 16 bit
	 */
	add_mem_device("sram0", MX31_CS4_BASE_ADDR, MX31_CS4_SIZE, /* area size */
				   IORESOURCE_MEM_WRITEABLE);
	imx31_add_nand(&nand_info);

	/*
	 * SMSC 9217 network controller
	 * connected to CS line 1 and interrupt line
	 * GPIO3, data width is 16 bit
	 */
	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL,	MX31_CS1_BASE_ADDR,
			MX31_CS1_SIZE, IORESOURCE_MEM, &smsc9217_pdata);

#ifdef CONFIG_USB
	pcm037_usb_init();
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX31_USB_OTG_BASE_ADDR, NULL);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX31_USB_HS2_BASE_ADDR, NULL);
#endif

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM037);

	return 0;
}

device_initcall(pcm037_devices_init);

static unsigned int pcm037_iomux[] = {
	/* UART1 */
	MX31_PIN_RXD1__RXD1,
	MX31_PIN_TXD1__TXD1,
	MX31_PIN_CTS1__CTS1,
	MX31_PIN_RTS1__RTS1,
	/* I2C */
	MX31_PIN_CSPI2_MOSI__SCL,
	MX31_PIN_CSPI2_MISO__SDA,
	MX31_PIN_CSPI2_SS2__I2C3_SDA,
	MX31_PIN_CSPI2_SCLK__I2C3_SCL,
	/* SDHC1 */
	MX31_PIN_SD1_DATA3__SD1_DATA3,
	MX31_PIN_SD1_DATA2__SD1_DATA2,
	MX31_PIN_SD1_DATA1__SD1_DATA1,
	MX31_PIN_SD1_DATA0__SD1_DATA0,
	MX31_PIN_SD1_CLK__SD1_CLK,
	MX31_PIN_SD1_CMD__SD1_CMD,
	IOMUX_MODE(MX31_PIN_SCK6, IOMUX_CONFIG_GPIO), /* card detect */
	IOMUX_MODE(MX31_PIN_SFS6, IOMUX_CONFIG_GPIO), /* write protect */
	/* SPI1 */
	MX31_PIN_CSPI1_MOSI__MOSI,
	MX31_PIN_CSPI1_MISO__MISO,
	MX31_PIN_CSPI1_SCLK__SCLK,
	MX31_PIN_CSPI1_SPI_RDY__SPI_RDY,
	MX31_PIN_CSPI1_SS0__SS0,
	MX31_PIN_CSPI1_SS1__SS1,
	MX31_PIN_CSPI1_SS2__SS2,
	/* UART2 */
	MX31_PIN_TXD2__TXD2,
	MX31_PIN_RXD2__RXD2,
	MX31_PIN_CTS2__CTS2,
	MX31_PIN_RTS2__RTS2,
	/* UART3 */
	MX31_PIN_CSPI3_MOSI__RXD3,
	MX31_PIN_CSPI3_MISO__TXD3,
	MX31_PIN_CSPI3_SCLK__RTS3,
	MX31_PIN_CSPI3_SPI_RDY__CTS3,
};

static int imx31_console_init(void)
{
	imx_iomux_setup_multiple_pins(pcm037_iomux, ARRAY_SIZE(pcm037_iomux));

	barebox_set_model("Phytec phyCORE-i.MX31");
	barebox_set_hostname("phycore-imx31");

	imx31_add_uart0();
	return 0;
}

console_initcall(imx31_console_init);
