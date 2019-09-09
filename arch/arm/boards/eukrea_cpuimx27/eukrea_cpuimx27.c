/*
 * Copyright (C) 2009 Eric Benard, Eukrea Electromatique
 * Based on pcm038.c which is :
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

#include <common.h>
#include <errno.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx27-regs.h>
#include <notifier.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <command.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <mach/weim.h>
#include <mach/imxfb.h>
#include <platform_data/serial-ns16550.h>
#include <asm/mmu.h>
#include <i2c/i2c.h>
#include <mfd/lp3972.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = 1,
};

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
	.flash_bbt = 1,
};

#ifdef CONFIG_DRIVER_SERIAL_NS16550
static struct NS16550_plat quad_uart_serial_plat = {
	.clock = 14745600,
	.shift = 1,
};

#ifdef CONFIG_EUKREA_CPUIMX27_QUART1
#define QUART_OFFSET 0x200000
#elif defined CONFIG_EUKREA_CPUIMX27_QUART2
#define QUART_OFFSET 0x400000
#elif defined CONFIG_EUKREA_CPUIMX27_QUART3
#define QUART_OFFSET 0x800000
#elif defined CONFIG_EUKREA_CPUIMX27_QUART4
#define QUART_OFFSET 0x1000000
#endif
#endif

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("lp3972", 0x34),
	},
};

#ifdef CONFIG_DRIVER_VIDEO_IMX
static struct fb_videomode imxfb_mode = {
	.name		= "CMO-QVGA",
	.refresh	= 60,
	.xres		= 320,
	.yres		= 240,
	.pixclock	= 156000,
	.hsync_len	= 30,
	.left_margin	= 38,
	.right_margin	= 20,
	.vsync_len	= 3,
	.upper_margin	= 15,
	.lower_margin	= 4,
};

static struct imx_fb_platform_data eukrea_cpuimx27_fb_data = {
	.mode	= &imxfb_mode,
	.num_modes = 1,
	.pwmr	= 0x00A903FF,
	.lscr1	= 0x00120300,
	.dmacr	= 0x00020010,
	.pcr	= 0xFAD08B80,
	.bpp	= 16,
};
#endif

static int eukrea_cpuimx27_devices_init(void)
{
	char *envdev = "no";
	int i;

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
		PD17_PF_I2C_DATA,
		PD18_PF_I2C_CLK,
#ifdef CONFIG_DRIVER_SERIAL_IMX
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
#endif
#ifdef CONFIG_DRIVER_VIDEO_IMX
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
		PA28_PF_HSYNC,
		PA29_PF_VSYNC,
		PA31_PF_OE_ACD,
		GPIO_PORTE | 5 | GPIO_GPIO | GPIO_OUT,
		GPIO_PORTA | 25 | GPIO_GPIO | GPIO_OUT,
#endif
	};

	/* configure 16 bit nor flash on cs0 */
	imx27_setup_weimcs(0, 0x00008F03, 0xA0330D01, 0x002208C0);

	/* initialize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx27_gpio_mode(mode[i]);

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC0000000, 32 * 1024 * 1024, 0);
#ifdef CONFIG_EUKREA_CPUIMX27_NOR_64MB
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC2000000, 32 * 1024 * 1024, 0);
#endif
	imx27_add_nand(&nand_info);

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	imx27_add_i2c0(NULL);

	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");
	protect_file("/dev/env0", 1);
	envdev = "NOR";

	printf("Using environment in %s Flash\n", envdev);

#ifdef CONFIG_DRIVER_VIDEO_IMX
	imx_add_fb((void *)0x10021000, &eukrea_cpuimx27_fb_data);
	gpio_direction_output(GPIO_PORTE | 5, 0);
	gpio_set_value(GPIO_PORTE | 5, 1);
	gpio_direction_output(GPIO_PORTA | 25, 0);
	gpio_set_value(GPIO_PORTA | 25, 1);
#endif

	armlinux_set_architecture(MACH_TYPE_EUKREA_CPUIMX27);

	return 0;
}

device_initcall(eukrea_cpuimx27_devices_init);

static int eukrea_cpuimx27_console_init(void)
{
	uint32_t val;

	barebox_set_model("Eukrea CPUIMX27");
	barebox_set_hostname("eukrea-cpuimx27");

#ifdef CONFIG_DRIVER_SERIAL_IMX
	imx27_add_uart0();
#endif
	/* configure 8 bit UART on cs3 */
	val = readl(MX27_SYSCTRL_BASE_ADDR + MX27_FMCR);
	val &= ~0x2;
	writel(val, MX27_SYSCTRL_BASE_ADDR + MX27_FMCR);

	imx27_setup_weimcs(3, 0x0000D603, 0x0D1D0D01, 0x00D20000);
#ifdef CONFIG_DRIVER_SERIAL_NS16550
	add_ns16550_device(DEVICE_ID_DYNAMIC, MX27_CS3_BASE_ADDR + QUART_OFFSET, 0xf,
			   IORESOURCE_MEM | IORESOURCE_MEM_16BIT,
			   &quad_uart_serial_plat);
#endif
	return 0;
}

console_initcall(eukrea_cpuimx27_console_init);

static int eukrea_cpuimx27_late_init(void)
{
#ifdef CONFIG_MFD_LP3972
	struct i2c_client *client;
	u8 reg[1];
#endif
	console_flush();
	imx27_add_fec(&fec_info);

#ifdef CONFIG_MFD_LP3972
	client = lp3972_get_client();
	if (!client)
		return -ENODEV;
	reg[0] = 0xa0;
	i2c_write_reg(client, 0x39, reg, sizeof(reg));
#endif
	return 0;
}

late_initcall(eukrea_cpuimx27_late_init);
