/*
 * (C) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
#include <partition.h>
#include <asm/mach-types.h>
#include <mach/imx-nand.h>
#include <fec.h>
#include <nand.h>
#include <mach/imx-flash-header.h>
#include <mach/iomux-mx25.h>

extern unsigned long _stext;

void __naked __flash_header_start go(void)
{
	__asm__ __volatile__("b _start\n");
}

struct imx_dcd_entry __dcd_entry_0x400 dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0xb8002050, .val = 0x0000d843, },
	{ .ptr_type = 4, .addr = 0xb8002054, .val = 0x22252521, },
	{ .ptr_type = 4, .addr = 0xb8002058, .val = 0x22220a00, },
	{ .ptr_type = 4, .addr = 0xb8001010, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x92100000, },
	{ .ptr_type = 1, .addr = 0x80000400, .val = 0x21, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xa2100000, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xb2100000, },
	{ .ptr_type = 1, .addr = 0x80000033, .val = 0xda, },
	{ .ptr_type = 1, .addr = 0x81000000, .val = 0xff, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x82216880, },
	{ .ptr_type = 4, .addr = 0xb8001004, .val = 0x00295729, },
	{ .ptr_type = 4, .addr = 0x53f80008, .val = 0x20034000, },
};

#define APP_DEST	0x80000000

struct imx_flash_header __flash_header_0x400 mx25_3ds_header = {
	.app_code_jump_vector	= APP_DEST + 0x1000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= APP_DEST,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof (dcd_entry),
};

extern unsigned long __bss_start;

unsigned long __image_len_0x400 barebox_len = 0x40000;

static struct fec_platform_data fec_info = {
	.xcv_type	= RMII,
	.phy_addr	= 1,
};

static struct device_d fec_dev = {
	.name     = "fec_imx",
	.map_base = IMX_FEC_BASE,
	.platform_data	= &fec_info,
};

static struct memory_platform_data sdram_pdata = {
	.name	= "ram0",
	.flags	= DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.name     = "mem",
	.map_base = IMX_SDRAM_CS0,
	.size     = 128 * 1024 * 1024,
	.platform_data = &sdram_pdata,
};

static struct memory_platform_data sram_pdata = {
	.name	= "sram0",
	.flags	= DEVFS_RDWR,
};

static struct device_d sram0_dev = {
	.name     = "mem",
	.map_base = 0x78000000,
	.size     = 128 * 1024,
	.platform_data = &sram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
};

static struct device_d nand_dev = {
	.name     = "imx_nand",
	.map_base = IMX_NFC_BASE,
	.platform_data	= &nand_info,
};

#ifdef CONFIG_USB
static void imx25_usb_init(void)
{
	unsigned int tmp;

	/* Host 2 */
	tmp = readl(IMX_OTG_BASE + 0x600);
	tmp &= ~(3 << 21);
	tmp |= (2 << 21) | (1 << 4) | (1 << 5);
	writel(tmp, IMX_OTG_BASE + 0x600);

	tmp = readl(IMX_OTG_BASE + 0x584);
	tmp |= 3 << 30;
	writel(tmp, IMX_OTG_BASE + 0x584);

	/* Set to Host mode */
	tmp = readl(IMX_OTG_BASE + 0x5a8);
	writel(tmp | 0x3, IMX_OTG_BASE + 0x5a8);
}

static struct device_d usbh2_dev = {
	.name     = "ehci",
	.map_base = IMX_OTG_BASE + 0x400,
	.size     = 0x200,
};
#endif

#define IOMUXC_BASE_ADDR        0x43FAC000

static int imx25_devices_init(void)
{
	ulong val;

	/*
	 * Set up the FEC_RESET_B and FEC_ENABLE GPIO pins.
	 * Assert FEC_RESET_B, then power up the PHY by asserting
	 * FEC_ENABLE, at the same time lifting FEC_RESET_B.
	 *
	 * FEC_RESET_B: gpio2[3] is ALT 5 mode of pin A17
	 * FEC_ENABLE_B: gpio4[8] is ALT 5 mode of pin D12
	 */
	writel(0x8, IOMUXC_BASE_ADDR + 0x0238); /* open drain */
	writel(0x0, IOMUXC_BASE_ADDR + 0x028C); /* cmos, no pu/pd */

#define GPIO2_BASE_ADDR         0x53FD0000
#define GPIO4_BASE_ADDR         0x53F9C000
#define GPIO_GDIR               0x04
#define GPIO_DR                 0x00

	/* make the pins output */
	val = (1 << 3) | readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	writel(val, GPIO2_BASE_ADDR + GPIO_GDIR);

	val = (1 << 8) | readl(GPIO4_BASE_ADDR + GPIO_GDIR);
	writel(val, GPIO4_BASE_ADDR + GPIO_GDIR);

	/* drop PHY power */
	val = readl(GPIO2_BASE_ADDR + GPIO_DR) & ~(1 << 3);
	writel(val, GPIO2_BASE_ADDR + GPIO_DR);

	/* assert reset */
	val = readl(GPIO4_BASE_ADDR + GPIO_DR) & ~(1 << 8);
	writel(val, GPIO4_BASE_ADDR + GPIO_DR);
	udelay(2);

	/* turn on power & lift reset */
	val = (1 << 3) | readl(GPIO2_BASE_ADDR + GPIO_DR);
	writel(val, GPIO2_BASE_ADDR + GPIO_DR);
	val = (1 << 8) | readl(GPIO4_BASE_ADDR + GPIO_DR);
	writel(val, GPIO4_BASE_ADDR + GPIO_DR);

#ifdef CONFIG_USB
	/* USB does not work yet. Don't know why. Maybe
	 * the CPLD has to be initialized.
	 */
	imx25_usb_init();
	register_device(&usbh2_dev);
#endif

	/* FEC does only work when the CPLD is initialized.
	 * Currently we do not do this in barebox, so it
	 * does only work when Linux has been started after
	 * the last powercycle.
	 */
	register_device(&fec_dev);

	if (readl(IMX_CCM_BASE + CCM_RCSR) & (1 << 14))
		nand_info.width = 2;

	register_device(&nand_dev);

	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	register_device(&sdram0_dev);
	register_device(&sram0_dev);

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_MX25_3DS);

	return 0;
}

device_initcall(imx25_devices_init);

static struct device_d imx25_serial_device = {
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 16 * 1024,
};

static struct pad_desc imx25_pads[] = {
	MX25_PAD_FEC_MDC__MDC,
	MX25_PAD_FEC_MDIO__MDIO,
	MX25_PAD_FEC_RDATA0__RDATA0,
	MX25_PAD_FEC_RDATA1__RDATA1,
	MX25_PAD_FEC_RX_DV__RX_DV,
	MX25_PAD_FEC_TDATA0__TDATA0,
	MX25_PAD_FEC_TDATA1__TDATA1,
	MX25_PAD_FEC_TX_CLK__TX_CLK,
	MX25_PAD_FEC_TX_EN__TX_EN,
	MX25_PAD_POWER_FAIL__POWER_FAIL_INT,
	MX25_PAD_A17__GPIO3,
	MX25_PAD_D12__GPIO8,
	/* UART1 */
	MX25_PAD_UART1_RXD__RXD_MUX,
	MX25_PAD_UART1_TXD__TXD_MUX,
	MX25_PAD_UART1_RTS__RTS,
	MX25_PAD_UART1_CTS__CTS,
	/* USBH2 */
	MX25_PAD_D9__USBH2_PWR,
	MX25_PAD_D8__USBH2_OC,
	MX25_PAD_LD0__USBH2_CLK,
	MX25_PAD_LD1__USBH2_DIR,
	MX25_PAD_LD2__USBH2_STP,
	MX25_PAD_LD3__USBH2_NXT,
	MX25_PAD_LD4__USBH2_DATA0,
	MX25_PAD_LD5__USBH2_DATA1,
	MX25_PAD_LD6__USBH2_DATA2,
	MX25_PAD_LD7__USBH2_DATA3,
	MX25_PAD_HSYNC__USBH2_DATA4,
	MX25_PAD_VSYNC__USBH2_DATA5,
	MX25_PAD_LSCLK__USBH2_DATA6,
	MX25_PAD_OE_ACD__USBH2_DATA7,
};

static int imx25_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(imx25_pads, ARRAY_SIZE(imx25_pads));

	writel(0x03010101, 0x53f80024);

	register_device(&imx25_serial_device);
	return 0;
}

console_initcall(imx25_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024, 2048, 16384);
}
#endif

static int imx25_core_setup(void)
{
	writel(0x01010103, IMX_CCM_BASE + CCM_PCDR2);
	return 0;

}
core_initcall(imx25_core_setup);

