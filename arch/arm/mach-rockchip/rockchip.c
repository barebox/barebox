// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <mach/rockchip.h>

static int rockchip_init(void)
{
	if (of_machine_is_compatible("rockchip,rk3188"))
		rk3188_init();
	else if (of_machine_is_compatible("rockchip,rk3288"))
		rk3288_init();
	else if (of_machine_is_compatible("rockchip,rk3568"))
		rk3568_init();
	else
		pr_err("Unknown rockchip SoC\n");

	return 0;
}
postcore_initcall(rockchip_init);
