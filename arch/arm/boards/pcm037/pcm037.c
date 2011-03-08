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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Board support for Phytec's, i.MX31 based CPU card, called: PCM037
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <fs.h>
#include <environment.h>
#include <usb/isp1504.h>
#include <mach/imx-regs.h>
#include <mach/iomux-mx31.h>
#include <asm/armlinux.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <partition.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <mach/devices-imx31.h>

/*
 * Up to 32MiB NOR type flash, connected to
 * CS line 0, data width is 16 bit
 */
static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = IMX_CS0_BASE,
	.size     = 32 * 1024 * 1024,	/* area size */
};

/*
 * up to 2MiB static RAM type memory, connected
 * to CS4, data width is 16 bit
 */
static struct memory_platform_data sram_dev_pdata0 = {
	.name = "sram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_CS4_BASE,
	.size     = IMX_CS4_RANGE,	/* area size */
	.platform_data = &sram_dev_pdata0,
};

/*
 * SMSC 9217 network controller
 * connected to CS line 1 and interrupt line
 * GPIO3, data width is 16 bit
 */
static struct device_d network_dev = {
	.id	  = -1,
	.name     = "smc911x",
	.map_base = IMX_CS1_BASE,
	.size     = IMX_CS1_RANGE,	/* area size */
};

#if defined CONFIG_PCM037_SDRAM_BANK0_128MB
#define SDRAM0	128
#elif defined CONFIG_PCM037_SDRAM_BANK0_256MB
#define SDRAM0	256
#endif

static struct memory_platform_data ram_dev_pdata0 = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS0,
	.size     = SDRAM0 * 1024 * 1024,	/* fix size */
	.platform_data = &ram_dev_pdata0,
};

#ifndef CONFIG_PCM037_SDRAM_BANK1_NONE

#if defined CONFIG_PCM037_SDRAM_BANK1_128MB
#define SDRAM1	128
#elif defined CONFIG_PCM037_SDRAM_BANK1_256MB
#define SDRAM1	256
#endif

static struct memory_platform_data ram_dev_pdata1 = {
	.name = "ram1",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram1_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS1,
	.size     = SDRAM1 * 1024 * 1024,	/* fix size */
	.platform_data = &ram_dev_pdata1,
};
#endif

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

static void pcm037_usb_init(void)
{
	u32 tmp;

	/* enable clock */
	tmp = readl(0x53f80000);
	tmp |= (1 << 9);
	writel(tmp, 0x53f80000);

	/* Host 1 */
	tmp = readl(IMX_OTG_BASE + 0x600);
	tmp &= ~((3 << 21) | 1);
	tmp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 11) | (1 << 20);
	writel(tmp, IMX_OTG_BASE + 0x600);

	tmp = readl(IMX_OTG_BASE + 0x184);
	tmp &= ~(3 << 30);
	tmp |= 2 << 30;
	writel(tmp, IMX_OTG_BASE + 0x184);

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
	isp1504_set_vbus_power((void *)(IMX_OTG_BASE + 0x170), 1);

	/* Host 2 */
	tmp = readl(IOMUXC_BASE + 0x8);
	tmp |= 1 << 11;
	writel(tmp, IOMUXC_BASE + 0x8);

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

	tmp = readl(IMX_OTG_BASE + 0x600);
	tmp &= ~((3 << 21) | 1);
	tmp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20);
	writel(tmp, IMX_OTG_BASE + 0x600);

	tmp = readl(IMX_OTG_BASE + 0x584);
	tmp &= ~(3 << 30);
	tmp |= 2 << 30;
	writel(tmp, IMX_OTG_BASE + 0x584);

	mdelay(50);
	isp1504_set_vbus_power((void *)(IMX_OTG_BASE + 0x570), 1);

	/* Set to Host mode */
	tmp = readl(IMX_OTG_BASE + 0x1a8);
	writel(tmp | 0x3, IMX_OTG_BASE + 0x1a8);

}
#endif

#ifdef CONFIG_MMU
static void pcm037_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 128, PMD_SECT_DEF_CACHED);
	arm_create_section(0x90000000, 0x80000000, 128, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

	mmu_enable();

#ifdef CONFIG_CACHE_L2X0
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);
#endif
}
#else
static void pcm037_mmu_init(void)
{
}
#endif

static int imx31_devices_init(void)
{
	pcm037_mmu_init();

	__REG(CSCR_U(0)) = 0x0000cf03; /* CS0: Nor Flash */
	__REG(CSCR_L(0)) = 0x10000d03;
	__REG(CSCR_A(0)) = 0x00720900;

	__REG(CSCR_U(1)) = 0x0000df06; /* CS1: Network Controller */
	__REG(CSCR_L(1)) = 0x444a4541;
	__REG(CSCR_A(1)) = 0x44443302;

	__REG(CSCR_U(4)) = 0x0000d843; /* CS4: SRAM */
	__REG(CSCR_L(4)) = 0x22252521;
	__REG(CSCR_A(4)) = 0x22220a00;

	__REG(CSCR_U(5)) = 0x0000DCF6; /* CS5: SJA1000 */
	__REG(CSCR_L(5)) = 0x444A0301;
	__REG(CSCR_A(5)) = 0x44443302;

	register_device(&cfi_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");	/* ourself */
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");	/* environment */

	protect_file("/dev/env0", 1);

	register_device(&sram_dev);
	imx31_add_nand(&nand_info);
	register_device(&network_dev);

	register_device(&sdram0_dev);
#ifndef CONFIG_PCM037_SDRAM_BANK1_NONE
	register_device(&sdram1_dev);
#endif
#ifdef CONFIG_USB
	pcm037_usb_init();
	register_device(&usbotg_dev);
	register_device(&usbh2_dev);
#endif

	armlinux_add_dram(&sdram0_dev);
#ifndef CONFIG_PCM037_SDRAM_BANK1_NONE
	armlinux_add_dram(&sdram1_dev);
#endif
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM037);

	return 0;
}

device_initcall(imx31_devices_init);

static int imx31_console_init(void)
{
	/* init gpios for serial port */
	imx_iomux_mode(MX31_PIN_RXD1__RXD1);
	imx_iomux_mode(MX31_PIN_TXD1__TXD1);
	imx_iomux_mode(MX31_PIN_CTS1__CTS1);
	imx_iomux_mode(MX31_PIN_RTS1__RTS1);

	imx31_add_uart0();
	return 0;
}

console_initcall(imx31_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif
