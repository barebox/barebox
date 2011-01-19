/*
 * i.MX23/28 USBPHY setup
 *
 * Copyright 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <common.h>
#include <asm/io.h>
#include <mach/imx-regs.h>

#define POWER_CTRL			(IMX_POWER_BASE + 0x0)
#define POWER_CTRL_CLKGATE		0x40000000

#define POWER_STS			(IMX_POWER_BASE + 0xc0)
#define POWER_STS_VBUSVALID		0x00000002
#define POWER_STS_BVALID		0x00000004
#define POWER_STS_AVALID		0x00000008

#define POWER_DEBUG			(IMX_POWER_BASE + 0x110)
#define POWER_DEBUG_BVALIDPIOLOCK	0x00000002
#define POWER_DEBUG_AVALIDPIOLOCK	0x00000004
#define POWER_DEBUG_VBUSVALIDPIOLOCK	0x00000008

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

int imx_usb_phy_enable(void)
{
	u32 reg;

	/*
	 * Set these bits so that we can force the OTG bits high
	 * so the ARC core operates properly
	 */
	writel(POWER_CTRL_CLKGATE, POWER_CTRL + CLR);

	writel(POWER_DEBUG_VBUSVALIDPIOLOCK |
			   POWER_DEBUG_AVALIDPIOLOCK |
			   POWER_DEBUG_BVALIDPIOLOCK, POWER_DEBUG + SET);

	reg = readl(POWER_STS);
	reg |= POWER_STS_BVALID | POWER_STS_AVALID | POWER_STS_VBUSVALID;
	writel(reg, POWER_STS);

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

