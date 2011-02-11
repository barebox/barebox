/*
 * (C) 2011 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <fec.h>
#include <nand.h>
#include <mach/iomux-mx25.h>
#include <mach/generic.h>
#include <linux/err.h>
#include <mach/devices-imx25.h>
#include <asm/mmu.h>

static struct fec_platform_data fec_info = {
	.xcv_type	= RMII,
	.phy_addr	= 0x1f,
};

static struct memory_platform_data sdram0_pdata = {
	.name	= "ram0",
	.flags	= DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS0,
	.size     = 32 * 1024 * 1024,
	.platform_data = &sdram0_pdata,
};

static struct memory_platform_data sdram1_pdata = {
	.name	= "ram1",
	.flags	= DEVFS_RDWR,
};

static struct device_d sdram1_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS1,
	.size     = 32 * 1024 * 1024,
	.platform_data = &sdram1_pdata,
};

static struct memory_platform_data sram_pdata = {
	.name	= "sram0",
	.flags	= DEVFS_RDWR,
};

static struct device_d sram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0x78000000,
	.size     = 128 * 1024,
	.platform_data = &sram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
	.flash_bbt = 1,
};

#ifdef CONFIG_MMU
static int tx25_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 32, PMD_SECT_DEF_CACHED);
	arm_create_section(0x82000000, 0x80000000, 32, PMD_SECT_DEF_UNCACHED);
	arm_create_section(0x90000000, 0x90000000, 32, PMD_SECT_DEF_CACHED);
	arm_create_section(0x92000000, 0x90000000, 32, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x02000000);

#if TEXT_BASE & (0x100000 - 1)
#warning cannot create vector section. Adjust TEXT_BASE to a 1M boundary
#else
	arm_create_section(0x0,        TEXT_BASE,   1, PMD_SECT_DEF_UNCACHED);
#endif
	mmu_enable();

	return 0;
}
postcore_initcall(tx25_mmu_init);
#endif

static struct pad_desc karo_tx25_padsd_fec[] = {
	MX25_PAD_D11__GPIO_4_9,		/* FEC PHY power on pin */
	MX25_PAD_D13__GPIO_4_7,		/* FEC reset */
	MX25_PAD_FEC_MDC__FEC_MDC,
	MX25_PAD_FEC_MDIO__FEC_MDIO,
	MX25_PAD_FEC_TDATA0__FEC_TDATA0,
	MX25_PAD_FEC_TDATA1__FEC_TDATA1,
	MX25_PAD_FEC_TX_EN__FEC_TX_EN,
	MX25_PAD_FEC_RDATA0__FEC_RDATA0,
	MX25_PAD_FEC_RDATA1__FEC_RDATA1,
	MX25_PAD_FEC_RX_DV__FEC_RX_DV,
	MX25_PAD_FEC_TX_CLK__FEC_TX_CLK,
};

#define TX25_FEC_PWR_GPIO	(GPIO_PORTD | 9)
#define TX25_FEC_RST_GPIO	(GPIO_PORTD | 7)

static void noinline gpio_fec_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(karo_tx25_padsd_fec,
			ARRAY_SIZE(karo_tx25_padsd_fec));

	/* power down phy, put into reset */
	gpio_direction_output(TX25_FEC_PWR_GPIO, 0);
	gpio_direction_output(TX25_FEC_RST_GPIO, 0);

	udelay(10);

	/* power up phy, get out of reset */
	gpio_direction_output(TX25_FEC_PWR_GPIO, 1);
	gpio_direction_output(TX25_FEC_RST_GPIO, 1);

	udelay(100);

	/* apply a reset to the powered phy again */
	gpio_direction_output(TX25_FEC_RST_GPIO, 0);
	udelay(100);
	gpio_direction_output(TX25_FEC_RST_GPIO, 1);
}

static int tx25_devices_init(void)
{
	gpio_fec_active();

	imx25_add_fec(&fec_info);

	if (readl(IMX_CCM_BASE + CCM_RCSR) & (1 << 14))
		nand_info.width = 2;

	imx25_add_nand(&nand_info);

	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x80000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	register_device(&sdram0_dev);
	register_device(&sdram1_dev);
	register_device(&sram0_dev);

	armlinux_add_dram(&sdram0_dev);
	armlinux_add_dram(&sdram1_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_TX25);
	armlinux_set_serial(imx_uid());

	return 0;
}

device_initcall(tx25_devices_init);

static struct pad_desc tx25_pads[] = {
	MX25_PAD_D12__GPIO_4_8,
	MX25_PAD_D10__GPIO_4_10,
	MX25_PAD_NF_CE0__NF_CE0,
	MX25_PAD_NFWE_B__NFWE_B,
	MX25_PAD_NFRE_B__NFRE_B,
	MX25_PAD_NFALE__NFALE,
	MX25_PAD_NFCLE__NFCLE,
	MX25_PAD_NFWP_B__NFWP_B,
	MX25_PAD_NFRB__NFRB,
	MX25_PAD_D7__D7,
	MX25_PAD_D6__D6,
	MX25_PAD_D5__D5,
	MX25_PAD_D4__D4,
	MX25_PAD_D3__D3,
	MX25_PAD_D2__D2,
	MX25_PAD_D1__D1,
	MX25_PAD_D0__D0,
	MX25_PAD_UART1_TXD__UART1_TXD,
	MX25_PAD_UART1_RXD__UART1_RXD,
	MX25_PAD_UART1_CTS__UART1_CTS,
	MX25_PAD_UART1_RTS__UART1_RTS,
};

static int tx25_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(tx25_pads, ARRAY_SIZE(tx25_pads));

	imx25_add_uart0();
	return 0;
}

console_initcall(tx25_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif

static struct pad_desc tx25_lcdc_gpios[] = {
	MX25_PAD_A18__GPIO_2_4,		/* LCD Reset (active LOW) */
	MX25_PAD_PWM__GPIO_1_26,	/* LCD Backlight brightness 0: full 1: off */
	MX25_PAD_A19__GPIO_2_5,		/* LCD Power Enable 0: off 1: on */
	MX25_PAD_LSCLK__LSCLK,
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
	MX25_PAD_D15__LD16,
	MX25_PAD_D14__LD17,
	MX25_PAD_HSYNC__HSYNC,
	MX25_PAD_VSYNC__VSYNC,
	MX25_PAD_OE_ACD__OE_ACD,
};

static struct imx_fb_videomode stk5_fb_mode = {
	.bpp	= 16,
	.mode = {
		.name = "G-ETV570G0DMU",
		.pixclock	= 33333,

		.xres		= 640,
		.yres		= 480,

		.hsync_len	= 64,
		.left_margin	= 96,
		.right_margin	= 80,

		.vsync_len	= 3,
		.upper_margin	= 46,
		.lower_margin	= 39,
	},
	.pcr	= PCR_TFT | PCR_COLOR | PCR_FLMPOL | PCR_LPPOL | PCR_SCLK_SEL,
};

#define STK5_LCD_BACKLIGHT_GPIO		(GPIO_PORTA | 26)
#define STK5_LCD_RESET_GPIO		(GPIO_PORTB | 4)
#define STK5_LCD_POWER_GPIO		(GPIO_PORTB | 5)

static void tx25_fb_enable(int enable)
{
	if (enable) {
		gpio_direction_output(STK5_LCD_RESET_GPIO, 1);
		gpio_direction_output(STK5_LCD_POWER_GPIO, 1);
		mdelay(300);
		gpio_direction_output(STK5_LCD_BACKLIGHT_GPIO, 0);
	} else {
		gpio_direction_output(STK5_LCD_BACKLIGHT_GPIO, 1);
		gpio_direction_output(STK5_LCD_RESET_GPIO, 0);
		gpio_direction_output(STK5_LCD_POWER_GPIO, 0);
	}
}

static struct imx_fb_platform_data tx25_fb_data = {
	.mode		= &stk5_fb_mode,
	.dmacr		= 0x80040060,
	.enable		= tx25_fb_enable,
};

static int tx25_init_fb(void)
{
	tx25_fb_enable(0);

	mxc_iomux_v3_setup_multiple_pads(tx25_lcdc_gpios,
			ARRAY_SIZE(tx25_lcdc_gpios));

	imx25_add_fb(&tx25_fb_data);

	return 0;
}
device_initcall(tx25_init_fb);
