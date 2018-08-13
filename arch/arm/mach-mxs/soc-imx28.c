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
#include <restart.h>
#include <mach/imx28-regs.h>
#include <io.h>
#include <asm/memory.h>
#include <mach/imx28.h>

#define HW_CLKCTRL_RESET 0x1e0
# define HW_CLKCTRL_RESET_CHIP (1 << 1)
#define HW_CLKCTRL_WDOG_POR_DISABLE (1 << 5)

/* Reset the full i.MX28 SoC via a chipset feature */
static void __noreturn imx28_restart_soc(struct restart_handler *rst)
{
	u32 reg;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_RESET);
	writel(reg | HW_CLKCTRL_RESET_CHIP, IMX_CCM_BASE + HW_CLKCTRL_RESET);

	hang();
	/*NOTREACHED*/
}

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

	restart_handler_register_fn(imx28_restart_soc);

	arm_add_mem_device("ram0", IMX_MEMORY_BASE, imx28_get_memsize());

	return 0;
}
postcore_initcall(imx28_init);

static int imx28_devices_init(void)
{
	if (of_get_root_node())
		return 0;

	add_generic_device("imx28-dma-apbh", 0, NULL, MXS_APBH_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-clkctrl", 0, NULL, IMX_CCM_BASE, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-gpio", 0, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-gpio", 1, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-gpio", 2, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-gpio", 3, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx28-gpio", 4, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("ocotp", 0, NULL, IMX_OCOTP_BASE, 0x2000, IORESOURCE_MEM, NULL);

	return 0;
}
postcore_initcall(imx28_devices_init);
