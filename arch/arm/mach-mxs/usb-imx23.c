// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* i.MX23 USBPHY setup */

#include <common.h>
#include <io.h>
#include <mach/imx23-regs.h>
#include <mach/power.h>
#include <mach/usb.h>

#define USBPHY_PWD			(IMX_USBPHY_BASE + 0x0)

#define USBPHY_CTRL			(IMX_USBPHY_BASE + 0x30)
#define USBPHY_CTRL_SFTRST		0x80000000
#define USBPHY_CTRL_CLKGATE		0x40000000

#define CLK_PLLCTRL0			(IMX_CCM_BASE + 0x0)
#define PLLCTRL0_EN_USB_CLKS 0x00040000

#define DIGCTRL_CTRL			(IMX_DIGCTL_BASE + 0x0)
#define DIGCTL_CTRL_USB_CLKGATE		0x00000004

#define SET	0x4
#define CLR	0x8

int imx23_usb_phy_enable(void)
{
	imx_power_prepare_usbphy();

	/* Reset USBPHY module */
	writel(USBPHY_CTRL_SFTRST, USBPHY_CTRL + SET);
	udelay(10);

	/* Remove CLKGATE and SFTRST */
	writel(USBPHY_CTRL_CLKGATE | USBPHY_CTRL_SFTRST, USBPHY_CTRL + CLR);

	/* Turn on the USB clocks */
	writel(PLLCTRL0_EN_USB_CLKS, CLK_PLLCTRL0 + SET);
	writel(DIGCTL_CTRL_USB_CLKGATE, DIGCTRL_CTRL + CLR);

	/* Power up the PHY */
	writel(0, USBPHY_PWD);

	/*
	 * Set precharge bit to cure overshoot problems at the
	 * start of packets
	 */
	writel(1, USBPHY_CTRL + SET);

	return 0;
}
