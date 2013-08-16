/*
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
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <sizes.h>
#include <gpio.h>
#include <platform_ide.h>
#include <mach/imx27-regs.h>
#include <mach/iomux-mx27.h>
#include <mach/weim.h>
#include <mach/devices-imx27.h>
#include <usb/chipidea-imx.h>

#define GPIO_IDE_POWER	(GPIO_PORTE + 18)
#define GPIO_IDE_PCOE	(GPIO_PORTF + 7)
#define GPIO_IDE_RESET	(GPIO_PORTF + 10)

static struct resource pcm970_ide_resources[] = {
	{
		.start	= MX27_PCMCIA_MEM_BASE_ADDR,
		.end	= MX27_PCMCIA_MEM_BASE_ADDR + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static void pcm970_ide_reset(int state)
{
	/* Switch reset line to low/high state */
	gpio_set_value(GPIO_IDE_RESET, !!state);
}

static struct ide_port_info pcm970_ide_pdata = {
	.ioport_shift	= 0,
	.reset		= &pcm970_ide_reset,
};

static struct device_d pcm970_ide_device = {
	.id		= DEVICE_ID_DYNAMIC,
	.name		= "ide_intf",
	.num_resources	= ARRAY_SIZE(pcm970_ide_resources),
	.resource	= pcm970_ide_resources,
	.platform_data	= &pcm970_ide_pdata,
};

static void pcm970_ide_init(void)
{
	uint32_t i;
	unsigned int mode[] = {
		/* PCMCIA */
		PF20_PF_PC_CD1,
		PF19_PF_PC_CD2,
		PF18_PF_PC_WAIT,
		PF17_PF_PC_READY,
		PF16_PF_PC_PWRON,
		PF14_PF_PC_VS1,
		PF13_PF_PC_VS2,
		PF12_PF_PC_BVD1,
		PF11_PF_PC_BVD2,
		PF9_PF_PC_IOIS16,
		PF8_PF_PC_RW,
		GPIO_IDE_PCOE | GPIO_GPIO | GPIO_OUT,	/* PCOE */
		GPIO_IDE_RESET | GPIO_GPIO | GPIO_OUT,	/* Reset */
		GPIO_IDE_POWER | GPIO_GPIO | GPIO_OUT,	/* Power */
	};

	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i] | GPIO_PUEN);

	/* Always set PCOE signal to low */
	gpio_set_value(GPIO_IDE_PCOE, 0);

	/* Assert RESET line */
	gpio_set_value(GPIO_IDE_RESET, 0);

	/* Power up CF-card (Also switched on User-LED) */
	gpio_set_value(GPIO_IDE_POWER, 1);
	mdelay(10);

	/* Reset PCMCIA Status Change Register */
	writel(0x00000fff, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PSCR);
	mdelay(10);

	/* Check PCMCIA Input Pins Register for Card Detect & Power */
	if ((readl(MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PIPR) &
				((1 << 8) | (3 << 3))) != (1 << 8)) {
		printf("CompactFlash card not found. Driver not enabled.\n");
		return;
	}

	/* Disable all interrupts */
	writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PER);

	/* Disable all PCMCIA banks */
	for (i = 0; i < 5; i++)
		writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(i));

	/* Not use internal PCOE */
	writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PGCR);

	/* Setup PCMCIA bank0 for Common memory mode */
	writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PBR(0));
	writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POFR(0));
	writel((0 << 25) | (17 << 17) | (4 << 11) | (3 << 5) | 0xf,
			MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(0));

	/* Clear PCMCIA General Status Register */
	writel(0x0000001f, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PGSR);

	/* Make PCMCIA bank0 valid */
	i = readl(MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(0));
	writel(i | (1 << 29), MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(0));

	platform_device_register(&pcm970_ide_device);
}

static void pcm970_mmc_init(void)
{
	uint32_t i;
	unsigned int mode[] = {
		/* SD2 */
		PB4_PF_SD2_D0,
		PB5_PF_SD2_D1,
		PB6_PF_SD2_D2,
		PB7_PF_SD2_D3,
		PB8_PF_SD2_CMD,
		PB9_PF_SD2_CLK,
	};

	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	imx27_add_mmc1(NULL);
}

struct imxusb_platformdata pcm970_usbh2_pdata = {
	.flags = MXC_EHCI_MODE_ULPI | MXC_EHCI_INTERFACE_DIFF_UNI,
	.mode = IMX_USB_MODE_HOST,
};

static int pcm970_init(void)
{
	int i;
	unsigned int mode[] = {
		/* USB Host 2 */
		PA0_PF_USBH2_CLK,
		PA1_PF_USBH2_DIR,
		PA2_PF_USBH2_DATA7,
		PA3_PF_USBH2_NXT,
		4 | GPIO_PORTA | GPIO_GPIO | GPIO_OUT,
		PD19_AF_USBH2_DATA4,
		PD20_AF_USBH2_DATA3,
		PD21_AF_USBH2_DATA6,
		PD22_AF_USBH2_DATA0,
		PD23_AF_USBH2_DATA2,
		PD24_AF_USBH2_DATA1,
		PD26_AF_USBH2_DATA5,
	};

	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	/* Configure SJA1000 on cs4 */
	imx27_setup_weimcs(4, 0x0000DCF6, 0x444A0301, 0x44443302);

	if (IS_ENABLED(CONFIG_USB)) {
		/* Stop ULPI */
		gpio_direction_output(4, 1);
		mdelay(1);
		imx_gpio_mode(PA4_PF_USBH2_STP);

		imx27_add_usbh2(&pcm970_usbh2_pdata);
	}

	if (IS_ENABLED(CONFIG_DISK_INTF_PLATFORM_IDE))
		pcm970_ide_init();

	if (IS_ENABLED(CONFIG_MCI_IMX))
		pcm970_mmc_init();

	return 0;
}

late_initcall(pcm970_init);
