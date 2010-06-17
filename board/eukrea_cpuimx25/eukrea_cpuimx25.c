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

#include <partition.h>
#include <asm/mach-types.h>
#include <mach/imx-nand.h>
#include <mach/imxfb.h>
#include <fec.h>
#include <nand.h>
#include <mach/imx-flash-header.h>
#include <mach/iomux-mx25.h>

extern unsigned long _stext;

void __naked __flash_header_start go(void)
{
	__asm__ __volatile__("b exception_vectors\n");
}

struct imx_dcd_entry __dcd_entry_0x400 dcd_entry[] = {
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

struct imx_flash_header __flash_header_0x400 eukrea_cpuimx25_header = {
	.app_code_jump_vector	= TEXT_BASE + 0x2000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= TEXT_BASE + 0x400 + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= TEXT_BASE + 0x400 + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= TEXT_BASE,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof(dcd_entry),
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
	.size     = 64 * 1024 * 1024,
	.platform_data = &sdram_pdata,
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


static struct device_d imxfb_dev = {
	.name		= "imxfb",
	.map_base	= 0x53fbc000,
	.size		= 0x1000,
	.platform_data	= &eukrea_cpuimx25_fb_data,
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
	MX25_PAD_FEC_MDC__MDC,
	MX25_PAD_FEC_MDIO__MDIO,
	MX25_PAD_FEC_RDATA0__RDATA0,
	MX25_PAD_FEC_RDATA1__RDATA1,
	MX25_PAD_FEC_RX_DV__RX_DV,
	MX25_PAD_FEC_TDATA0__TDATA0,
	MX25_PAD_FEC_TDATA1__TDATA1,
	MX25_PAD_FEC_TX_CLK__TX_CLK,
	MX25_PAD_FEC_TX_EN__TX_EN,
	/* UART1 */
	MX25_PAD_UART1_RXD__RXD_MUX,
	MX25_PAD_UART1_TXD__TXD_MUX,
	MX25_PAD_UART1_RTS__RTS,
	MX25_PAD_UART1_CTS__CTS,
	/* LCDC */
	MX25_PAD_LD0__LCDC_LD0,
	MX25_PAD_LD1__LCDC_LD1,
	MX25_PAD_LD2__LCDC_LD2,
	MX25_PAD_LD3__LCDC_LD3,
	MX25_PAD_LD4__LCDC_LD4,
	MX25_PAD_LD5__LCDC_LD5,
	MX25_PAD_LD6__LCDC_LD6,
	MX25_PAD_LD7__LCDC_LD7,
	MX25_PAD_LD8__LCDC_LD8,
	MX25_PAD_LD9__LCDC_LD9,
	MX25_PAD_LD10__LCDC_LD10,
	MX25_PAD_LD11__LCDC_LD11,
	MX25_PAD_LD12__LCDC_LD12,
	MX25_PAD_LD13__LCDC_LD13,
	MX25_PAD_LD14__LCDC_LD14,
	MX25_PAD_LD15__LCDC_LD15,
	MX25_PAD_GPIO_E__LCDC_LD16,
	MX25_PAD_GPIO_F__LCDC_LD17,
	MX25_PAD_LSCLK__LCDC_LSCLK,
	MX25_PAD_OE_ACD__LCDC_OE_ACD,
	MX25_PAD_VSYNC__LCDC_VSYN,
	MX25_PAD_HSYNC__LCDC_HSYN,
	/* BACKLIGHT CONTROL */
	MX25_PAD_PWM__GPIO26,
};

static int eukrea_cpuimx25_devices_init(void)
{
	eukrea_cpuimx25_mmu_init();

	mxc_iomux_v3_setup_multiple_pads(eukrea_cpuimx25_pads,
		ARRAY_SIZE(eukrea_cpuimx25_pads));
	register_device(&fec_dev);

	nand_info.width = 1;
	register_device(&nand_dev);

	devfs_add_partition("nand0", 0x00000, 0x40000,
		PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x40000,
		PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	register_device(&sdram0_dev);

	/* enable LCD */
	gpio_direction_output(26, 1);
	gpio_set_value(26, 1);

	register_device(&imxfb_dev);

	armlinux_add_dram(&sdram0_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_EUKREA_CPUIMX25);

	return 0;
}

device_initcall(eukrea_cpuimx25_devices_init);

static struct device_d eukrea_cpuimx25_serial_device = {
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 16 * 1024,
};

static int eukrea_cpuimx25_console_init(void)
{
	writel(0x03010101, IMX_CCM_BASE + CCM_PCDR3);
	register_device(&eukrea_cpuimx25_serial_device);
	return 0;
}

console_initcall(eukrea_cpuimx25_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif

static int eukrea_cpuimx25_core_setup(void)
{
	writel(0x01010103, IMX_CCM_BASE + CCM_PCDR2);
	return 0;

}
core_initcall(eukrea_cpuimx25_core_setup);
