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
#include <mach/imx-regs.h>
#include <mach/iomux-mx27.h>
#include <mach/gpio.h>
#include <usb/ulpi.h>

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

	return 0;
}

late_initcall(pcm970_init);
