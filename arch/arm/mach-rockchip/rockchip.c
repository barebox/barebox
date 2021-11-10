// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <mach/rockchip.h>

static int rockchip_init(void)
{
	if (of_machine_is_compatible("rockchip,rk3188"))
		return rk3188_init();
	if (of_machine_is_compatible("rockchip,rk3288"))
		return rk3288_init();
	if (of_machine_is_compatible("rockchip,rk3566"))
		return rk3568_init();
	if (of_machine_is_compatible("rockchip,rk3568"))
		return rk3568_init();

	pr_err("Unknown rockchip SoC\n");
	return -ENODEV;
}
postcore_initcall(rockchip_init);
