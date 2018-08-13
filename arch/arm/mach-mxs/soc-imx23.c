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
#include <mach/imx23-regs.h>
#include <io.h>
#include <asm/memory.h>
#include <mach/imx23.h>

#define HW_CLKCTRL_RESET 0x120
# define HW_CLKCTRL_RESET_CHIP (1 << 1)

/* Reset the full i.MX23 SoC via a chipset feature */
static void __noreturn imx23_restart_soc(struct restart_handler *rst)
{
	u32 reg;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_RESET);
	writel(reg | HW_CLKCTRL_RESET_CHIP, IMX_CCM_BASE + HW_CLKCTRL_RESET);

	hang();
	/*NOTREACHED*/
}

static int imx23_devices_init(void)
{
	arm_add_mem_device("ram0", IMX_MEMORY_BASE, imx23_get_memsize());

	if (of_get_root_node())
		return 0;

	add_generic_device("imx23-dma-apbh", 0, NULL, MXS_APBH_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx23-clkctrl", 0, NULL, IMX_CCM_BASE, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx23-gpio", 0, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx23-gpio", 1, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	add_generic_device("imx23-gpio", 2, NULL, IMX_IOMUXC_BASE, 0x2000, IORESOURCE_MEM, NULL);
	restart_handler_register_fn(imx23_restart_soc);

	return 0;
}
postcore_initcall(imx23_devices_init);
