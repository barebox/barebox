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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <sizes.h>
#include <platform_ide.h>
#include <mach/imx-regs.h>
#include <mach/iomux-mx27.h>
#include <mach/gpio.h>
#include <mach/devices-imx27.h>
#include <usb/ulpi.h>

#define GPIO_IDE_POWER	(GPIO_PORTE + 18)
#define GPIO_IDE_PCOE	(GPIO_PORTF + 7)
#define GPIO_IDE_RESET	(GPIO_PORTF + 10)

#ifdef CONFIG_USB
static void pcm970_usbh2_init(void)
{
	uint32_t temp;

	temp = readl(IMX_OTG_BASE + 0x600);
	temp &= ~((3 << 21) | 1);
	temp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20);
	writel(temp, IMX_OTG_BASE + 0x600);

	temp = readl(IMX_OTG_BASE + 0x584);
	temp &= ~(3 << 30);
	temp |= 2 << 30;
	writel(temp, IMX_OTG_BASE + 0x584);

	mdelay(10);

	if (!ulpi_setup((void *)(IMX_OTG_BASE + 0x570), 1))
		add_generic_usb_ehci_device(-1, IMX_OTG_BASE + 0x400, NULL);
}
#endif

#ifdef CONFIG_DISK_INTF_PLATFORM_IDE
static struct resource pcm970_ide_resources[] = {
	{
		.start	= IMX_PCMCIA_MEM_BASE,
		.end	= IMX_PCMCIA_MEM_BASE + SZ_1K - 1,
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
	.id		= -1,
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
	writel(0x00000fff, PCMCIA_PSCR);
	mdelay(10);

	/* Check PCMCIA Input Pins Register for Card Detect & Power */
	if ((readl(PCMCIA_PIPR) & ((1 << 8) | (3 << 3))) != (1 << 8)) {
		printf("CompactFlash card not found. Driver not enabled.\n");
		return;
	}

	/* Disable all interrupts */
	writel(0, PCMCIA_PER);

	/* Disable all PCMCIA banks */
	for (i = 0; i < 5; i++)
		writel(0, PCMCIA_POR(i));

	/* Not use internal PCOE */
	writel(0, PCMCIA_PGCR);

	/* Setup PCMCIA bank0 for Common memory mode */
	writel(0, PCMCIA_PBR(0));
	writel(0, PCMCIA_POFR(0));
	writel((0 << 25) | (17 << 17) | (4 << 11) | (3 << 5) | 0xf, PCMCIA_POR(0));

	/* Clear PCMCIA General Status Register */
	writel(0x0000001f, PCMCIA_PGSR);

	/* Make PCMCIA bank0 valid */
	writel(readl(PCMCIA_POR(0)) | (1 << 29), PCMCIA_POR(0));

	register_device(&pcm970_ide_device);
}
#endif

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

	PCCR0 |= PCCR0_SDHC2_EN;
	imx27_add_mmc1(NULL);
}

static int pcm970_init(void)
{
	int i;
	unsigned int mode[] = {
		/* USB Host 2 */
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
	};

	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	/* Configure SJA1000 on cs4 */
	imx27_setup_weimcs(4, 0x0000DCF6, 0x444A0301, 0x44443302);

#ifdef CONFIG_USB
	pcm970_usbh2_init();
#endif

#ifdef CONFIG_DISK_INTF_PLATFORM_IDE
	pcm970_ide_init();
#endif

	if (IS_ENABLED(CONFIG_MCI_IMX))
		pcm970_mmc_init();

	return 0;
}

late_initcall(pcm970_init);
