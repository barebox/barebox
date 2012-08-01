/*
 * Copyright (C) 2009 Ivo Clarysse
 * 
 * Based on imx27ads.c,
 *   Copyright (C) 2007 Sascha Hauer, Pengutronix 
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
#include <asm/armlinux.h>
#include <asm-generic/sections.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <mach/gpio.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <mach/imxfb.h>
#include <mach/iomux-mx21.h>
#include <mach/devices-imx21.h>

#define MX21ADS_IO_REG    0xCC800000
#define MX21ADS_IO_LCDON  (1 << 9)

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
};

/* Sharp LQ035Q7DB02 QVGA display */
static struct imx_fb_videomode imx_fb_modedata = {
        .mode = {
		.name           = "Sharp-LQ035Q7",
		.refresh        = 60,
		.xres           = 240,
		.yres           = 320,
		.pixclock       = 188679,
		.left_margin    = 6,
		.right_margin   = 16,
		.upper_margin   = 8,
		.lower_margin   = 10,
		.hsync_len      = 2,
		.vsync_len      = 1,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED,
		.flag           = 0,
	},
        .pcr            = 0xfb108bc7,
        .bpp            = 16,
};

static struct imx_fb_platform_data imx_fb_data = {
	.mode           = &imx_fb_modedata,
	.num_modes	= 1,
	.cmap_greyscale = 0,
	.cmap_inverse   = 0,
	.cmap_static    = 0,
	.pwmr           = 0x00a903ff,
	.lscr1          = 0x00120300,
	.dmacr          = 0x00020008,
};

static int imx21ads_timing_init(void)
{
	u32 temp;

	/* Configure External Interface Module */
	/* CS0: burst flash */
	CS0U = 0x00003E00;
	CS0L = 0x00000E01;

	/* CS1: Ethernet controller, external UART, memory-mapped I/O (16-bit) */
	CS1U = 0x00002000;
	CS1L = 0x11118501;

	/* CS2: disable (not available, since CSD0 in use) */
	CS2U = 0x0;
	CS2L = 0x0;

	/* CS3: disable */
	CS3U = 0x0;
	CS3L = 0x0;
	/* CS4: disable */
	CS4U = 0x0;
	CS4L = 0x0;
	/* CS5: disable */
	CS5U = 0x0;
	CS5L = 0x0;

	temp = PCDR0;
	temp &= ~0xF000;
	temp |= 0xA000;  /* Set NFC divider; 0xA yields 24.18MHz */
	PCDR0 = temp;

	return 0;
}

core_initcall(imx21ads_timing_init);

static int mx21ads_mem_init(void)
{
	arm_add_mem_device("ram0", 0xc0000000, 64 * 1024 * 1024);

	return 0;
}
mem_initcall(mx21ads_mem_init);

static int mx21ads_devices_init(void)
{
	int i;
	unsigned int mode[] = {
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
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
	};

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	add_cfi_flash_device(-1, 0xC8000000, 32 * 1024 * 1024, 0);
	imx21_add_nand(&nand_info);
	add_generic_device("cs8900", DEVICE_ID_DYNAMIC, NULL, IMX_CS1_BASE, 0x1000,
			IORESOURCE_MEM, NULL);
	imx21_add_fb(&imx_fb_data);

	armlinux_set_bootparams((void *)0xc0000100);
	armlinux_set_architecture(MACH_TYPE_MX21ADS);

	return 0;
}

device_initcall(mx21ads_devices_init);

static int mx21ads_enable_display(void)
{
	u16 tmp;

	tmp = readw(MX21ADS_IO_REG);
	tmp |= MX21ADS_IO_LCDON;
	writew(tmp, MX21ADS_IO_REG);
	return 0;
}

late_initcall(mx21ads_enable_display);

static int mx21ads_console_init(void)
{
	imx21_add_uart0();
	return 0;
}

console_initcall(mx21ads_console_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	PCCR0 |= PCCR0_NFC_EN;
	imx_nand_load_image(_text, barebox_image_size);
	board_init_lowlevel_return();
}
#endif

