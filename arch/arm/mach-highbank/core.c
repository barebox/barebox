/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <io.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/amba/bus.h>

#include <asm/hardware/arm_timer.h>

#include <mach/devices.h>
#include <mach/hardware.h>
#include <mach/sysregs.h>

void __iomem *sregs_base = IOMEM(HB_SREG_A9_BASE);

static void highbank_clk_init(void)
{
	struct clk *clk;

	clk = clk_fixed("dummy_apb_pclk", 0);
	clk_register_clkdev(clk, "apb_pclk", NULL);

	clk = clk_fixed("pclk", 150000000);
	clk_register_clkdev(clk, NULL, "sp804");
	clk_register_clkdev(clk, NULL, "uart-pl011");
}

static void highbank_sp804_init(void)
{
	amba_apb_device_add(NULL, "sp804", DEVICE_ID_SINGLE, 0xfff34000, 4096, NULL, 0);
}

static int highbank_init(void)
{
	highbank_clk_init();
	highbank_sp804_init();

	return 0;
}
postcore_initcall(highbank_init);
