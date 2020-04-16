/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <mach/omap4-mux.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-generic.h>
#include <mach/omap4-clock.h>
#include <mach/syslib.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>

#include "mux.h"

#define TPS62361_VSEL0_GPIO    7

static const struct ddr_regs ddr_regs_400_mhz_2cs = {
	/* tRRD changed from 10ns to 12.5ns because of the tFAW requirement*/
	.tim1		= 0x10eb0662,
	.tim2		= 0x20370dd2,
	.tim3		= 0x00b1c33f,
	.phy_ctrl_1	= 0x849FF408,
	.ref_ctrl	= 0x00000618,
	.config_init	= 0x80000eb9,
	.config_final	= 0x80001ab9,
	.zq_config	= 0xD00b3215,
	.mr1		= 0x83,
	.mr2		= 0x4
};

static void noinline panda_init_lowlevel(void)
{
	struct dpll_param core = OMAP4_CORE_DPLL_PARAM_38M4_DDR400;
	struct dpll_param mpu = OMAP4_MPU_DPLL_PARAM_38M4_MPU600;
	struct dpll_param iva = OMAP4_IVA_DPLL_PARAM_38M4;
	struct dpll_param per = OMAP4_PER_DPLL_PARAM_38M4;
	struct dpll_param abe = OMAP4_ABE_DPLL_PARAM_38M4;
	struct dpll_param usb = OMAP4_USB_DPLL_PARAM_38M4;
	unsigned int rev = omap4_revision();

	writel(CM_SYS_CLKSEL_38M4, CM_SYS_CLKSEL);

	/* Configure all DPLL's at 100% OPP */
	omap4_configure_mpu_dpll(&mpu);
	omap4_configure_iva_dpll(&iva);
	omap4_configure_per_dpll(&per);
	omap4_configure_abe_dpll(&abe);
	omap4_configure_usb_dpll(&usb);

	/* Enable all clocks */
	omap4_enable_all_clocks();

	panda_set_muxconf_regs();

	omap4_ddr_init(&ddr_regs_400_mhz_2cs, &core);

	if (rev < OMAP4460_ES1_0)
		omap4430_scale_vcores();
	else
		omap4460_scale_vcores(TPS62361_VSEL0_GPIO, 1210);
}

void __bare_init __naked barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	omap4_save_bootinfo((void *)r0);

	arm_cpu_lowlevel_init();

	if (get_pc() > 0x80000000)
		goto out;

	arm_setup_stack(0x4030d000);

	panda_init_lowlevel();
out:
	barebox_arm_entry(0x80000000, SZ_1G, NULL);
}
