/*
 * Author: Carlo Caione <carlo@carlocaione.org>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
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
 */

#include <common.h>
#include <init.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <io.h>
#include <asm/armlinux.h>
#include <sizes.h>

#include <mach/platform.h>
#include <mach/wd.h>
#include <mach/core.h>
#include <linux/amba/bus.h>

enum brcm_clks {
	dummy, clk_ref_3, clk_ref_1, clks_max
};

static struct clk *clks[clks_max];

static int bcm2835_clk_init(void)
{
	int ret;

	clks[dummy] = clk_fixed("dummy", 0);
	clks[clk_ref_3] = clk_fixed("ref3", 3 * 1000 * 1000);
	clks[clk_ref_1] = clk_fixed("ref1", 1 * 1000 * 1000);

	ret = clk_register_clkdev(clks[dummy], "apb_pclk", NULL);
	if (ret)
		goto clk_err;

	ret = clk_register_clkdev(clks[clk_ref_3], NULL, "uart0-pl0110");
	if (ret)
		goto clk_err;

	ret = clk_register_clkdev(clks[clk_ref_1], NULL, "bcm2835-cs");
	if (ret)
		goto clk_err;

	return 0;

clk_err:
	return ret;

}
postcore_initcall(bcm2835_clk_init);

static int bcm2835_dev_init(void)
{
	add_generic_device("bcm2835-gpio", 0, NULL, BCM2835_GPIO_BASE, 0xB0, IORESOURCE_MEM, NULL);
	add_generic_device("bcm2835-cs", DEVICE_ID_SINGLE, NULL, BCM2835_ST_BASE, 0x1C, IORESOURCE_MEM, NULL);
	add_generic_device("bcm2835_mci", 0, NULL, BCM2835_EMMC_BASE, 0xFC, IORESOURCE_MEM, NULL);
	return 0;
}
coredevice_initcall(bcm2835_dev_init);

void bcm2835_register_uart(void)
{
	amba_apb_device_add(NULL, "uart0-pl011", 0, BCM2835_UART0_BASE, 4096, NULL, 0);
}

void bcm2835_add_device_sdram(u32 size)
{
	if (!size)
		size = get_ram_size((ulong *) BCM2835_SDRAM_BASE, SZ_128M);

	arm_add_mem_device("ram0", BCM2835_SDRAM_BASE, size);
}
#define RESET_TIMEOUT 10

void __noreturn reset_cpu (unsigned long addr)
{
	uint32_t rstc;

	rstc = readl(PM_RSTC);
	rstc &= ~PM_RSTC_WRCFG_SET;
	rstc |= PM_RSTC_WRCFG_FULL_RESET;
	writel(PM_PASSWORD | RESET_TIMEOUT, PM_WDOG);
	writel(PM_PASSWORD | rstc, PM_RSTC);

	while (1);
}
EXPORT_SYMBOL(reset_cpu);
