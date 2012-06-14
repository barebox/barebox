/*
 * i.MX28 power related functions
 *
 * Copyright 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 * Copyright (C) 2012 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <common.h>
#include <io.h>
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

void imx_power_prepare_usbphy(void)
{
	u32 reg;

	/*
	 * Set these bits so that we can force the OTG bits high
	 * so the ARC core operates properly
	 */
	writel(POWER_CTRL_CLKGATE, POWER_CTRL + BIT_CLR);

	writel(POWER_DEBUG_VBUSVALIDPIOLOCK |
			   POWER_DEBUG_AVALIDPIOLOCK |
			   POWER_DEBUG_BVALIDPIOLOCK, POWER_DEBUG + BIT_SET);

	reg = readl(POWER_STS);
	reg |= POWER_STS_BVALID | POWER_STS_AVALID | POWER_STS_VBUSVALID;
	writel(reg, POWER_STS);
}
