/*
 * Copyright (C) 2016 PHYTEC Messtechnik GmbH,
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>
#include <common.h>
#include <init.h>
#include <restart.h>
#include <reset_source.h>
#include <bootsource.h>
#include <mach/rk3288-regs.h>
#include <mach/cru_rk3288.h>
#include <mach/hardware.h>

static void __noreturn rockchip_restart_soc(struct restart_handler *rst)
{
	struct rk3288_cru *cru = (struct rk3288_cru *)RK3288_CRU_BASE;

	/* cold reset */
	writel(RK_CLRBITS(0xffff), &cru->cru_mode_con);
	writel(0xfdb9, &cru->cru_glb_srst_fst_value);

	hang();
}

static void rk3288_detect_reset_reason(void)
{
	struct rk3288_cru *cru = (struct rk3288_cru *)RK3288_CRU_BASE;

	switch (cru->cru_glb_rst_st) {
	case (1 << 0):
		reset_source_set(RESET_POR);
		break;
	case (1 << 1):
		reset_source_set(RESET_RST);
		break;
	case (1 << 2):
	case (1 << 3):
		reset_source_set(RESET_THERM);
		break;
	case (1 << 4):
	case (1 << 5):
		reset_source_set(RESET_WDG);
		break;
	default:
		reset_source_set(RESET_UKWN);
		break;
	}
}

static int rk3288_init(void)
{
	restart_handler_register_fn(rockchip_restart_soc);

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		rk3288_detect_reset_reason();

	return 0;
}
postcore_initcall(rk3288_init);

/*
 * ATM we are not able to determine the boot source.
 * So let's handle the environment on eMMC, regardless which device
 * we are booting from.
 */
static int rk3288_env_init(void)
{
	const char *envpath = "/chosen/environment-emmc";
	int ret;

	bootsource_set(BOOTSOURCE_MMC);
	bootsource_set_instance(0);

	ret = of_device_enable_path(envpath);
	if (ret < 0)
		pr_warn("Failed to enable environment partition '%s' (%d)\n",
			envpath, ret);

	return 0;
}
device_initcall(rk3288_env_init);
