/*
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
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
 * Foundation.
 *
 */

#include <common.h>
#include <io.h>
#include <mach/imx6-regs.h>
#include <mach/usb.h>

#define SET				0x4
#define CLR				0x8

#define USBPHY_CTRL			0x30
#define USB_OTG_CTRL			0x800
#define USB_UH1_CTRL			0x804
#define USB_UH2_CTRL			0x808
#define USB_UH3_CTRL			0x80c

#define USB_UH1_USBCMD			0x340

#define USB_CMD_RUNSTOP			(1 <<  0)

#define USB_OVER_CUR_DIS		(1 <<  7)
#define USBPHY_CTRL_SFTRST		(1 << 31)
#define USBPHY_CTRL_CLKGATE		(1 << 30)
#define USBPHY_CTRL_ENUTMILEVEL3	(1 << 15)
#define USBPHY_CTRL_ENUTMILEVEL2	(1 << 14)

#define USBPHY1_PLL_480_CTRL_EN		(1 << 13)
#define USBPHY1_PLL_480_CTRL_POWER	(1 << 12)
#define USBPHY1_PLL_480_CTRL_EN_USB_CLK	(1 <<  6)
#define USBPHY1_PLL_480_CTRL_BYPASS	(1 << 16)

int imx6_usb_phy2_disable_oc(void)
{
	int val;

	/* disable over current detection */
	val = readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH1_CTRL);
	val |= USB_OVER_CUR_DIS;
	writel(val, MX6_USBOH3_USB_BASE_ADDR + USB_UH1_CTRL);
	val = readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH2_CTRL);
	val |= USB_OVER_CUR_DIS;
	writel(val, MX6_USBOH3_USB_BASE_ADDR + USB_UH2_CTRL);
	val = readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH3_CTRL);
	val |= USB_OVER_CUR_DIS;
	writel(val, MX6_USBOH3_USB_BASE_ADDR + USB_UH3_CTRL);

	return 0;
}

int imx6_usb_phy2_enable(void)
{
	int val;

	/* disable external charger detector or DP will be poor */
	writel(0x00180000, MX6_ANATOP_BASE_ADDR + 0x1b0);
	writel(0x00180000, MX6_ANATOP_BASE_ADDR + 0x210);

	/* enable usb pll */
	writel(USBPHY1_PLL_480_CTRL_EN |
	       USBPHY1_PLL_480_CTRL_POWER |
	       USBPHY1_PLL_480_CTRL_EN_USB_CLK, MX6_ANATOP_BASE_ADDR + 0x24);

	/* turn OFF clk bypass */
	/* at least on imx6 v1.0 this essential for usb to work */
	/* FIXME: test on v1.1. Datasheet declares bit as reserved */
	writel(USBPHY1_PLL_480_CTRL_BYPASS, MX6_ANATOP_BASE_ADDR + 0x28);

	/* stop then reset */
	val = readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD);
	val &= ~USB_CMD_RUNSTOP;
	writel(val, MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD);
	while (readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD) & USB_CMD_RUNSTOP);

	val = readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD);
	val |= USB_CMD_RESET;
	writel(val, MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD);
	while (readl(MX6_USBOH3_USB_BASE_ADDR + USB_UH1_USBCMD) & USB_CMD_RESET);

	/* reset usbphy */
	writel(USBPHY_CTRL_SFTRST, MX6_USBPHY2_BASE_ADDR + USBPHY_CTRL + SET);
	udelay(10);
	/* clr reset and clkgate */
	writel(USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE, MX6_USBPHY2_BASE_ADDR + USBPHY_CTRL + CLR);

	/* clr all pwd bits => power up phy */
	writel(0xffffffff, MX6_USBPHY2_BASE_ADDR + CLR);

	/* set utmilvl2/3 */
	val = readl(MX6_USBPHY2_BASE_ADDR + USBPHY_CTRL);
	val |= USBPHY_CTRL_ENUTMILEVEL3 | USBPHY_CTRL_ENUTMILEVEL2;
	writel(val, MX6_USBPHY2_BASE_ADDR + USBPHY_CTRL + SET);

	return 0;
}
