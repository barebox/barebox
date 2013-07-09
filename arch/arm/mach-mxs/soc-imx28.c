/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
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
 * Collection of some SoC specific functions
 */

#include <common.h>
#include <init.h>
#include <mach/imx-regs.h>
#include <io.h>

#define HW_CLKCTRL_RESET 0x1e0
# define HW_CLKCTRL_RESET_CHIP (1 << 1)
#define HW_CLKCTRL_WDOG_POR_DISABLE (1 << 5)

/* Reset the full i.MX28 SoC via a chipset feature */
void __noreturn reset_cpu(unsigned long addr)
{
	u32 reg;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_RESET);
	writel(reg | HW_CLKCTRL_RESET_CHIP, IMX_CCM_BASE + HW_CLKCTRL_RESET);

	while (1)
		;
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);

static int imx28_init(void)
{
	u32 reg;

	/*
	 * The default setting for the WDT is to do a POR. If the SoC is only
	 * powered via battery, then a WDT reset powers the chip down instead
	 * of resetting it. Use a software reset only.
	 */
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_RESET) |
		HW_CLKCTRL_WDOG_POR_DISABLE;
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_RESET);

	return 0;
}
postcore_initcall(imx28_init);

static int imx28_devices_init(void)
{
	add_generic_device("imx28-dma-apbh", 0, NULL, MXS_APBH_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-clkctrl", 0, NULL, IMX_CCM_BASE, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
postcore_initcall(imx28_devices_init);
