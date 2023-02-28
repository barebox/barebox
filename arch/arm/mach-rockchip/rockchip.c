// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <mach/rockchip/rockchip.h>

static int __rockchip_soc = -1;

int rockchip_soc(void)
{
	return __rockchip_soc;
}

static int rockchip_init(void)
{
	if (of_machine_is_compatible("rockchip,rk3188")) {
		__rockchip_soc = 3188;
		return rk3188_init();
	}

	if (of_machine_is_compatible("rockchip,rk3288")) {
		__rockchip_soc = 3288;
		return rk3288_init();
	}

	if (of_machine_is_compatible("rockchip,rk3566")) {
		__rockchip_soc = 3566;
		return rk3568_init();
	}

	if (of_machine_is_compatible("rockchip,rk3568")) {
		__rockchip_soc = 3568;
		return rk3568_init();
	}

	return 0;
}
postcore_initcall(rockchip_init);
