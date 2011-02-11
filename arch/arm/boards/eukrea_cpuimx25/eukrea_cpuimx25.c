/*
 * (C) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (c) 2010 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <environment.h>
#include <mach/imx-regs.h>
#include <asm/armlinux.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <led.h>

#include <partition.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <mach/imxfb.h>
#include <fec.h>
#include <nand.h>
#include <mach/imx-flash-header.h>
#include <mach/iomux-mx25.h>
#include <i2c/i2c.h>
#include <usb/fsl_usb2.h>
#include <mach/usb.h>
#include <mach/devices-imx25.h>

extern unsigned long _stext;
extern void exception_vectors(void);

void __naked __flash_header_start go(void)
{
	__asm__ __volatile__("b exception_vectors\n");
}

struct imx_dcd_entry __dcd_entry_section dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0xb8001010, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x92100000, },
	{ .ptr_type = 1, .addr = 0x80000400, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xa2100000, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xb2100000, },
	{ .ptr_type = 1, .addr = 0x80000033, .val = 0xda, },
	{ .ptr_type = 1, .addr = 0x81000000, .val = 0xff, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x82216080, },
	{ .ptr_type = 4, .addr = 0xb8001004, .val = 0x00295729, },
	{ .ptr_type = 4, .addr = 0x53f80008, .val = 0x20034000, },
};

struct imx_flash_header __flash_header_section flash_header = {
	.app_code_jump_vector	= DEST_BASE + ((unsigned int)&exception_vectors - TEXT_BASE),
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= DEST_BASE,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof(dcd_entry),
};

extern unsigned long __bss_start;

unsigned long __image_len_section barebox_len = 0x40000;

static struct fec_platform_data fec_info = {
	.xcv_type	= RMII,
	.phy_addr	= 1,
};

static struct memory_platform_data sdram_pdata = {
	.name	= "ram0",
	.flags	= DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS0,
	.size     = 64 * 1024 * 1024,
	.platform_data = &sdram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
};

static struct imx_fb_videomode imxfb_mode = {
	.mode = {
		.name		= "CMO-QVGA",
		.refresh	= 60,
		.xres		= 320,
		.yres		= 240,
		.pixclock	= KHZ2PICOS(6500),
		.hsync_len	= 30,
		.left_margin	= 38,
		.right_margin	= 20,
		.vsync_len	= 3,
		.upper_margin	= 15,
		.lower_margin	= 4,
	},
	.pcr		= 0xCAD08B80,
	.bpp		= 16,
};

static struct imx_fb_platform_data eukrea_cpuimx25_fb_data = {
	.mode		= &imxfb_mode,
	.pwmr		= 0x00A903FF,
	.lscr1		= 0x00120300,
	.dmacr		= 0x80040060,
};

struct gpio_led led0 = {
	.gpio = 2 * 32 + 19,
	.active_low = 1,
};

#ifdef CONFIG_USB
static void imx25_usb_init(void)
{
	unsigned int tmp;

	/* Host 1 */
	tmp = readl(IMX_OTG_BASE + 0x600);
	tmp &= ~(MX35_H1_SIC_MASK | MX35_H1_PM_BIT | MX35_H1_TLL_BIT |
		MX35_H1_USBTE_BIT | MX35_H1_IPPUE_DOWN_BIT | MX35_H1_IPPUE_UP_BIT);
	tmp |= (MXC_EHCI_INTERFACE_SINGLE_UNI) << MX35_H1_SIC_SHIFT;
	tmp |= MX35_H1_USBTE_BIT;
	tmp |= MX35_H1_IPPUE_DOWN_BIT;
	writel(tmp, IMX_OTG_BASE + 0x600);

	tmp = readl(IMX_OTG_BASE + 0x584);
	tmp |= 3 << 30;
	writel(tmp, IMX_OTG_BASE + 0x584);

	/* Set to Host mode */
	tmp = readl(IMX_OTG_BASE + 0x5a8);
	writel(tmp | 0x3, IMX_OTG_BASE + 0x5a8);
}

static struct device_d usbh2_dev = {
	.id	  = -1,
	.name     = "ehci",
	.map_base = IMX_OTG_BASE + 0x400,
	.size     = 0x200,
};
#endif

static struct fsl_usb2_platform_data usb_pdata = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_UTMI,
};

static struct device_d usbotg_dev = {
	.name     = "fsl-udc",
	.map_base = IMX_OTG_BASE,
	.size     = 0x200,
	.platform_data = &usb_pdata,
};

#ifdef CONFIG_MMU
static void eukrea_cpuimx25_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 128, PMD_SECT_DEF_CACHED);
	arm_create_section(0x90000000, 0x80000000, 128, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

	mmu_enable();
}
#else
static void eukrea_cpuimx25_mmu_init(void)
{
}
#endif

static struct pad_desc eukrea_cpuimx25_pads[] = {
	MX25_PAD_FEC_MDC__FEC_MDC,
	MX25_PAD_FEC_MDIO__FEC_MDIO,
	MX25_PAD_FEC_RDATA0__FEC_RDATA0,
	MX25_PAD_FEC_RDATA1__FEC_RDATA1,
	MX25_PAD_FEC_RX_DV__FEC_RX_DV,
	MX25_PAD_FEC_TDATA0__FEC_TDATA0,
	MX25_PAD_FEC_TDATA1__FEC_TDATA1,
	MX25_PAD_FEC_TX_CLK__FEC_TX_CLK,
	MX25_PAD_FEC_TX_EN__FEC_TX_EN,
	/* UART1 */
	MX25_PAD_UART1_RXD__UART1_RXD,
	MX25_PAD_UART1_TXD__UART1_TXD,
	MX25_PAD_UART1_RTS__UART1_RTS,
	MX25_PAD_UART1_CTS__UART1_CTS,
	/* LCDC */
	MX25_PAD_LD0__LD0,
	MX25_PAD_LD1__LD1,
	MX25_PAD_LD2__LD2,
	MX25_PAD_LD3__LD3,
	MX25_PAD_LD4__LD4,
	MX25_PAD_LD5__LD5,
	MX25_PAD_LD6__LD6,
	MX25_PAD_LD7__LD7,
	MX25_PAD_LD8__LD8,
	MX25_PAD_LD9__LD9,
	MX25_PAD_LD10__LD10,
	MX25_PAD_LD11__LD11,
	MX25_PAD_LD12__LD12,
	MX25_PAD_LD13__LD13,
	MX25_PAD_LD14__LD14,
	MX25_PAD_LD15__LD15,
	MX25_PAD_GPIO_E__LD16,
	MX25_PAD_GPIO_F__LD17,
	MX25_PAD_LSCLK__LSCLK,
	MX25_PAD_OE_ACD__OE_ACD,
	MX25_PAD_VSYNC__VSYNC,
	MX25_PAD_HSYNC__HSYNC,
	/* BACKLIGHT CONTROL */
	MX25_PAD_PWM__GPIO_1_26,
	/* I2C */
	MX25_PAD_I2C1_CLK__I2C1_CLK,
	MX25_PAD_I2C1_DAT__I2C1_DAT,
	/* SDCard */
	MX25_PAD_SD1_CLK__SD1_CLK,
	MX25_PAD_SD1_CMD__SD1_CMD,
	MX25_PAD_SD1_DATA0__SD1_DATA0,
	MX25_PAD_SD1_DATA1__SD1_DATA1,
	MX25_PAD_SD1_DATA2__SD1_DATA2,
	MX25_PAD_SD1_DATA3__SD1_DATA3,
	/* LED */
	MX25_PAD_POWER_FAIL__GPIO_3_19,
};

static int eukrea_cpuimx25_devices_init(void)
{
	eukrea_cpuimx25_mmu_init();

	mxc_iomux_v3_setup_multiple_pads(eukrea_cpuimx25_pads,
		ARRAY_SIZE(eukrea_cpuimx25_pads));

	led_gpio_register(&led0);

	imx25_add_fec(&fec_info);

	nand_info.width = 1;
	imx25_add_nand(&nand_info);

	devfs_add_partition("nand0", 0x00000, 0x40000,
		PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000,
		PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	register_device(&sdram0_dev);

	/* enable LCD */
	gpio_direction_output(26, 1);
	gpio_set_value(26, 1);

	/* LED : default OFF */
	gpio_direction_output(2 * 32 + 19, 1);

	imx25_add_fb(&eukrea_cpuimx25_fb_data);

	imx25_add_i2c0(NULL);
	imx25_add_mmc0(NULL);

#ifdef CONFIG_USB
	imx25_usb_init();
	register_device(&usbh2_dev);
#endif
	register_device(&usbotg_dev);

	armlinux_add_dram(&sdram0_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_EUKREA_CPUIMX25);

	return 0;
}

device_initcall(eukrea_cpuimx25_devices_init);

static int eukrea_cpuimx25_console_init(void)
{
	imx25_add_uart0();
	return 0;
}

console_initcall(eukrea_cpuimx25_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif

static int eukrea_cpuimx25_core_init(void) {
	/* enable UART1, FEC, SDHC, USB & I2C clock */
	writel(readl(IMX_CCM_BASE + CCM_CGCR0) | (1 << 6) | (1 << 23)
		| (1 << 15) | (1 << 21) | (1 << 3) | (1 << 28),
		IMX_CCM_BASE + CCM_CGCR0);
	writel(readl(IMX_CCM_BASE + CCM_CGCR1) | (1 << 23) | (1 << 15)
		| (1 << 13), IMX_CCM_BASE + CCM_CGCR1);
	writel(readl(IMX_CCM_BASE + CCM_CGCR2) | (1 << 14),
		IMX_CCM_BASE + CCM_CGCR2);

	return 0;
}

core_initcall(eukrea_cpuimx25_core_init);
