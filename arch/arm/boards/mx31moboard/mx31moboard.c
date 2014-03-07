/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) 2014 EPFL, Philippe Rétornaz <philippe.retornaz@epfl.ch>
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
 * Board support for EPFL's, i.MX31 based CPU card
 *
 * Based on:
 * Board support for Phytec's, i.MX31 based CPU card, called: PCM037
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <fs.h>
#include <gpio.h>
#include <led.h>
#include <environment.h>
#include <usb/ulpi.h>
#include <mach/imx31-regs.h>
#include <mach/iomux-mx31.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <mach/weim.h>
#include <io.h>
#include <asm/mmu.h>
#include <partition.h>
#include <generated/mach-types.h>
#include <asm/barebox-arm.h>
#include <mach/devices-imx31.h>

#define USBH2_EN_B	IOMUX_TO_GPIO(MX31_PIN_SCK6)
#define USB_RESET_B	IOMUX_TO_GPIO(MX31_PIN_GPIO1_0)

static void mx31moboard_usb_init(void)
{
	u32 tmp;

	if (!IS_ENABLED(CONFIG_USB))
		return;

	/* enable clock */
	tmp = readl(0x53f80000);
	tmp |= (1 << 9);
	writel(tmp, 0x53f80000);

	/* Host 2 */
	tmp = readl(MX31_IOMUXC_GPR);
	tmp |= 1 << 11;	/* IOMUX GPR: enable USBH2 signals */
	writel(tmp, MX31_IOMUXC_GPR);

	imx_iomux_mode(IOMUX_MODE(MX31_PIN_SCK6, IOMUX_CONFIG_GPIO));
	imx_iomux_mode(IOMUX_MODE(MX31_PIN_GPIO1_0, IOMUX_CONFIG_GPIO));
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

#define H2_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS \
			| PAD_CTL_ODE_CMOS)
	imx_iomux_set_pad(MX31_PIN_USBH2_CLK, H2_PAD_CFG | PAD_CTL_100K_PU);
	imx_iomux_set_pad(MX31_PIN_USBH2_DIR, H2_PAD_CFG | PAD_CTL_100K_PU);
	imx_iomux_set_pad(MX31_PIN_USBH2_NXT, H2_PAD_CFG | PAD_CTL_100K_PU);
	imx_iomux_set_pad(MX31_PIN_USBH2_STP, H2_PAD_CFG | PAD_CTL_100K_PU);
	imx_iomux_set_pad(MX31_PIN_USBH2_DATA0, H2_PAD_CFG); /* USBH2_DATA0 */
	imx_iomux_set_pad(MX31_PIN_USBH2_DATA1, H2_PAD_CFG); /* USBH2_DATA1 */
	imx_iomux_set_pad(MX31_PIN_SRXD6, H2_PAD_CFG);	/* USBH2_DATA2 */
	imx_iomux_set_pad(MX31_PIN_STXD6, H2_PAD_CFG);	/* USBH2_DATA3 */
	imx_iomux_set_pad(MX31_PIN_SFS3, H2_PAD_CFG);	/* USBH2_DATA4 */
	imx_iomux_set_pad(MX31_PIN_SCK3, H2_PAD_CFG);	/* USBH2_DATA5 */
	imx_iomux_set_pad(MX31_PIN_SRXD3, H2_PAD_CFG);	/* USBH2_DATA6 */
	imx_iomux_set_pad(MX31_PIN_STXD3, H2_PAD_CFG);	/* USBH2_DATA7 */


	gpio_request(USB_RESET_B, "usb-reset");
	gpio_direction_output(USB_RESET_B, 0);
	mdelay(5);
	gpio_set_value(USB_RESET_B, 1);
	mdelay(10);

	gpio_request(USBH2_EN_B, "usbh2-en");
	gpio_direction_output(USBH2_EN_B, 0);
	udelay(900);
	gpio_set_value(USBH2_EN_B, 1);
	udelay(200);

	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x600);
	tmp &= ~((3 << 21) | 1);
	tmp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20);
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x600);

	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x584);
	tmp &= ~(3 << 30);
	tmp |= 2 << 30;
	writel(tmp, MX31_USB_OTG_BASE_ADDR + 0x584);

	gpio_set_value(USBH2_EN_B, 0);

	mdelay(50);

	ulpi_setup((void *)(MX31_USB_OTG_BASE_ADDR + 0x570), 1);

	/* Set to Host mode */
	tmp = readl(MX31_USB_OTG_BASE_ADDR + 0x1a8);
	writel(tmp | 0x3, MX31_USB_OTG_BASE_ADDR + 0x1a8);

}

static struct gpio_led mx31moboard_leds[] = {
	{
		.led = {
			.name = "coreboard-led-0:red:running"
		},
		.gpio = IOMUX_TO_GPIO(MX31_PIN_SVEN0),
	}, {
		.led = {
			.name   = "coreboard-led-1:red",
		},
		.gpio = IOMUX_TO_GPIO(MX31_PIN_STX0),
	}, {
		.led = {
			.name   = "coreboard-led-2:red",
		},
		.gpio   = IOMUX_TO_GPIO(MX31_PIN_SRX0),
	}, {
		.led = {
			.name   = "coreboard-led-3:red",
		},
		.gpio = IOMUX_TO_GPIO(MX31_PIN_SIMPD0),
	},
};

static void mx31moboard_add_leds(void)
{
	int i;

	if (!IS_ENABLED(CONFIG_LED_GPIO))
		return;

	for (i = 0; i < ARRAY_SIZE(mx31moboard_leds); i++) {
		led_gpio_register(&mx31moboard_leds[i]);
		led_set(&mx31moboard_leds[i].led, 0);
	}

	led_set_trigger(LED_TRIGGER_HEARTBEAT, &mx31moboard_leds[0].led);
}

static int mx31moboard_mmu_init(void)
{
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);

	return 0;
}
postmmu_initcall(mx31moboard_mmu_init);

static const struct devfs_partition mx31moboard_nor0_partitions[] = {
	{
		.offset = 0,
		.size = SZ_512K,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "self0",
	}, {
		.offset = DEVFS_PARTITION_APPEND,
		.size = SZ_256K,
		.name = "env0",
	}, {
		/* Sentinel */
	}
};

static int mx31moboard_devices_init(void)
{
	/* CS0: Nor Flash */
	imx31_setup_weimcs(0, 0x0000CC03, 0xa0330D01, 0x00220800);

	/*
	 * Up to 32MiB NOR type flash, connected to
	 * CS line 0, data width is 16 bit
	 */
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, MX31_CS0_BASE_ADDR, SZ_32M, 0);

	imx31_add_mmc0(NULL);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_create_partitions("nor0", mx31moboard_nor0_partitions);
	protect_file("/dev/env0", 1);

	mx31moboard_usb_init();
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC,
					MX31_USB_HS2_BASE_ADDR, NULL);

	mx31moboard_add_leds();

	armlinux_set_architecture(MACH_TYPE_MX31MOBOARD);

	return 0;
}

device_initcall(mx31moboard_devices_init);

static unsigned int mx31moboard_iomux[] = {
	/* UART1 */
	MX31_PIN_RXD1__RXD1,
	MX31_PIN_TXD1__TXD1,
	MX31_PIN_CTS1__GPIO2_7,
	/* SDHC1 */
	MX31_PIN_SD1_DATA3__SD1_DATA3,
	MX31_PIN_SD1_DATA2__SD1_DATA2,
	MX31_PIN_SD1_DATA1__SD1_DATA1,
	MX31_PIN_SD1_DATA0__SD1_DATA0,
	MX31_PIN_SD1_CLK__SD1_CLK,
	MX31_PIN_SD1_CMD__SD1_CMD,
	MX31_PIN_ATA_CS0__GPIO3_26, MX31_PIN_ATA_CS1__GPIO3_27,
	/* LEDS */
	MX31_PIN_SVEN0__GPIO2_0, MX31_PIN_STX0__GPIO2_1,
	MX31_PIN_SRX0__GPIO2_2, MX31_PIN_SIMPD0__GPIO2_3,
};

static int imx31_console_init(void)
{
	imx_iomux_setup_multiple_pins(mx31moboard_iomux,
					ARRAY_SIZE(mx31moboard_iomux));

	gpio_request(IOMUX_TO_GPIO(MX31_PIN_CTS1), "uart0-cts-hack");
	gpio_direction_output(IOMUX_TO_GPIO(MX31_PIN_CTS1), 0);

	barebox_set_model("EPFL mx31moboard");
	barebox_set_hostname("mx31moboard");

	imx31_add_uart0();

	return 0;
}

console_initcall(imx31_console_init);
