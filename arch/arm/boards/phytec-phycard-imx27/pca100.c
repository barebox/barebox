// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 Sascha Hauer, Pengutronix

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/imx27-regs.h>
#include <gpio.h>
#include <linux/sizes.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <asm/mach-types.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <io.h>
#include <mach/imx/imx-nand.h>
#include <mach/imx/imx-pll.h>
#include <platform_data/imxfb.h>
#include <asm/mmu.h>
#include <linux/usb/ulpi.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx27.h>
#include <mach/imx/devices-imx27.h>

#if defined(CONFIG_USB) && defined(CONFIG_USB_ULPI)
static void pca100_usb_register(void)
{
	mdelay(10);

	gpio_direction_output(GPIO_PORTB + 24, 0);
	gpio_direction_output(GPIO_PORTB + 23, 0);

	mdelay(10);

	ulpi_setup((void *)(MX27_USB_OTG_BASE_ADDR + 0x170), 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX27_USB_OTG_BASE_ADDR, NULL);
	ulpi_setup((void *)(MX27_USB_OTG_BASE_ADDR + 0x570), 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX27_USB_OTG_BASE_ADDR + 0x400, NULL);
}
#else
static void pca100_usb_register(void) { };
#endif

static void pca100_usb_init(void)
{
	u32 reg;

	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x600);
	reg &= ~((3 << 21) | 1);
	reg |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 11) | (1 << 20);
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x600);

	/*
	 * switch usbotg and usbh2 to ulpi mode. Do this *before*
	 * the iomux setup to prevent funny hardware bugs from
	 * triggering. Also, do this even when USB support is
	 * disabled to give Linux USB support a good start.
	 */
	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x584);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x584);

	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x184);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x184);

	/* disable the usb phys */
	imx27_gpio_mode((GPIO_PORTB | 23) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 23, 1);
	imx27_gpio_mode((GPIO_PORTB | 24) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 24, 1);
}

static int pca100_devices_init(void)
{
	int i;
	unsigned int mode[] = {
		/* USB host 2 */
		PA0_PF_USBH2_CLK,
		PA1_PF_USBH2_DIR,
		PA2_PF_USBH2_DATA7,
		PA3_PF_USBH2_NXT,
		PA4_PF_USBH2_STP,
		PD19_AF_USBH2_DATA4,
		PD20_AF_USBH2_DATA3,
		PD21_AF_USBH2_DATA6,
		PD22_AF_USBH2_DATA0,
		PD23_AF_USBH2_DATA2,
		PD24_AF_USBH2_DATA1,
		PD26_AF_USBH2_DATA5,
		PC7_PF_USBOTG_DATA5,
		PC8_PF_USBOTG_DATA6,
		PC9_PF_USBOTG_DATA0,
		PC10_PF_USBOTG_DATA2,
		PC11_PF_USBOTG_DATA1,
		PC12_PF_USBOTG_DATA4,
		PC13_PF_USBOTG_DATA3,
		PE0_PF_USBOTG_NXT,
		PE1_PF_USBOTG_STP,
		PE2_PF_USBOTG_DIR,
		PE24_PF_USBOTG_CLK,
		PE25_PF_USBOTG_DATA7,
	};

	if (!of_machine_is_compatible("phytec,imx27-pca100"))
		return 0;

	barebox_set_model("Phytec phyCARD-i.MX27");
	barebox_set_hostname("phycard-imx27");

	pca100_usb_init();

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx27_gpio_mode(mode[i]);

	pca100_usb_register();

	imx_bbu_external_nand_register_handler("nand", "/dev/nand0.boot",
			BBU_HANDLER_FLAG_DEFAULT);

	armlinux_set_architecture(2149);

	return 0;
}

device_initcall(pca100_devices_init);
